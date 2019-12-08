/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Engine.hpp"

#include "BlockFactory.hpp"
#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "ControlBindings.hpp"
#include "DirectDriver.hpp"
#include "Driver.hpp"
#include "Event.hpp"
#include "EventWriter.hpp"
#include "GraphImpl.hpp"
#include "LV2Options.hpp"
#include "PostProcessor.hpp"
#include "PreProcessor.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"
#include "UndoStack.hpp"
#include "Worker.hpp"
#include "events/CreateGraph.hpp"
#include "ingen_config.h"

#ifdef HAVE_SOCKET
#include "SocketListener.hpp"
#endif

#include "ingen/AtomReader.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Log.hpp"
#include "ingen/Store.hpp"
#include "ingen/StreamWriter.hpp"
#include "ingen/Tee.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "lv2/buf-size/buf-size.h"
#include "lv2/state/state.h"
#include "raul/Maid.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <thread>
#include <utility>

namespace ingen {
namespace server {

INGEN_THREAD_LOCAL unsigned ThreadManager::flags(0);
bool               ThreadManager::single_threaded(true);

Engine::Engine(ingen::World& world)
	: _world(world)
	, _options(new LV2Options(world.uris()))
	, _buffer_factory(new BufferFactory(*this, world.uris()))
	, _maid(new Raul::Maid)
	, _worker(new Worker(world.log(), event_queue_size()))
	, _sync_worker(new Worker(world.log(), event_queue_size(), true))
	, _broadcaster(new Broadcaster())
	, _control_bindings(new ControlBindings(*this))
	, _block_factory(new BlockFactory(world))
	, _undo_stack(new UndoStack(world.uris(), world.uri_map()))
	, _redo_stack(new UndoStack(world.uris(), world.uri_map()))
	, _post_processor(new PostProcessor(*this))
	, _pre_processor(new PreProcessor(*this))
	, _event_writer(new EventWriter(*this))
	, _interface(_event_writer)
	, _atom_interface(
		new AtomReader(world.uri_map(), world.uris(), world.log(), *_interface))
	, _root_graph(nullptr)
	, _cycle_start_time(0)
	, _rand_engine(0)
	, _uniform_dist(0.0f, 1.0f)
	, _quit_flag(false)
	, _reset_load_flag(false)
	, _atomic_bundles(world.conf().option("atomic-bundles").get<int32_t>())
	, _activated(false)
{
	if (!world.store()) {
		world.set_store(std::make_shared<ingen::Store>());
	}

	for (int i = 0; i < world.conf().option("threads").get<int32_t>(); ++i) {
		_notifications.emplace_back(
			make_unique<Raul::RingBuffer>(uint32_t(24 * event_queue_size())));
		_run_contexts.emplace_back(
			make_unique<RunContext>(
				*this, _notifications.back().get(), unsigned(i), i > 0));
	}

	_world.lv2_features().add_feature(_worker->schedule_feature());
	_world.lv2_features().add_feature(_options);
	_world.lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_BUF_SIZE__powerOf2BlockLength)));
	_world.lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_BUF_SIZE__fixedBlockLength)));
	_world.lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_BUF_SIZE__boundedBlockLength)));
	_world.lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_STATE__loadDefaultState)));

	if (world.conf().option("dump").get<int32_t>()) {
		_interface = std::make_shared<Tee>(
			Tee::Sinks{
				_event_writer,
				std::make_shared<StreamWriter>(world.uri_map(),
				                               world.uris(),
				                               URI("ingen:/engine"),
				                               stderr,
				                               ColorContext::Color::MAGENTA)});
	}
}

Engine::~Engine()
{
	_root_graph = nullptr;
	deactivate();

	// Process all pending events
	const FrameTime end = std::numeric_limits<FrameTime>::max();
	RunContext&     ctx = run_context();
	locate(ctx.end(), end - ctx.end());
	_post_processor->set_end_time(end);
	_post_processor->process();
	while (!_pre_processor->empty()) {
		_pre_processor->process(ctx, *_post_processor, 1);
		_post_processor->process();
	}

	_atom_interface.reset();

	// Delete run contexts
	_quit_flag = true;
	_tasks_available.notify_all();
	for (const auto& ctx : _run_contexts) {
		ctx->join();
	}

	const SPtr<Store> store = this->store();
	if (store) {
		for (auto& s : *store.get()) {
			if (!dynamic_ptr_cast<NodeImpl>(s.second)->parent()) {
				s.second.reset();
			}
		}
		store->clear();
	}

	_world.set_store(SPtr<ingen::Store>());
}

void
Engine::listen()
{
#ifdef HAVE_SOCKET
	_listener = UPtr<SocketListener>(new SocketListener(*this));
#endif
}

void
Engine::advance(SampleCount nframes)
{
	for (const auto& ctx : _run_contexts) {
		ctx->locate(ctx->start() + nframes, block_length());
	}
}

void
Engine::locate(FrameTime s, SampleCount nframes)
{
	for (const auto& ctx : _run_contexts) {
		ctx->locate(s, nframes);
	}
}

void
Engine::set_root_graph(GraphImpl* graph)
{
	_root_graph = graph;
}

void
Engine::flush_events(const std::chrono::milliseconds& sleep_ms)
{
	bool finished = !pending_events();
	while (!finished) {
		// Run one audio block to execute prepared events
		run(block_length());
		advance(block_length());

		// Run one main iteration to post-process events
		main_iteration();

		// Sleep before continuing if there are still events to process
		if (!(finished = !pending_events())) {
			std::this_thread::sleep_for(sleep_ms);
		}
	}
}

void
Engine::emit_notifications(FrameTime end)
{
	for (const auto& ctx : _run_contexts) {
		ctx->emit_notifications(end);
	}
}

bool
Engine::pending_notifications()
{
	for (const auto& ctx : _run_contexts) {
		if (ctx->pending_notifications()) {
			return true;
		}
	}
	return false;
}

bool
Engine::wait_for_tasks()
{
	if (!_quit_flag) {
		std::unique_lock<std::mutex> lock(_tasks_mutex);
		_tasks_available.wait(lock);
	}
	return !_quit_flag;
}

void
Engine::signal_tasks_available()
{
	_tasks_available.notify_all();
}

Task*
Engine::steal_task(unsigned start_thread)
{
	for (unsigned i = 0; i < _run_contexts.size(); ++i) {
		const unsigned id  = (start_thread + i) % _run_contexts.size();
		const auto&    ctx = _run_contexts[id];
		Task*          par = ctx->task();
		if (par) {
			Task* t = par->steal(*ctx);
			if (t) {
				return t;
			}
		}
	}
	return nullptr;
}

SPtr<Store>
Engine::store() const
{
	return _world.store();
}

SampleRate
Engine::sample_rate() const
{
	return _driver->sample_rate();
}

SampleCount
Engine::block_length() const
{
	return _driver->block_length();
}

size_t
Engine::sequence_size() const
{
	return _driver->seq_size();
}

size_t
Engine::event_queue_size() const
{
	return _world.conf().option("queue-size").get<int32_t>();
}

void
Engine::quit()
{
	_quit_flag = true;
}

Properties
Engine::load_properties() const
{
	const ingen::URIs& uris = _world.uris();

	return { { uris.ingen_meanRunLoad,
		       uris.forge.make(floorf(_run_load.mean) / 100.0f) },
		     { uris.ingen_minRunLoad,
	           uris.forge.make(_run_load.min / 100.0f) },
		     { uris.ingen_maxRunLoad,
		       uris.forge.make(_run_load.max / 100.0f) } };
}

bool
Engine::main_iteration()
{
	_post_processor->process();
	_maid->cleanup();

	if (_run_load.changed) {
		_broadcaster->put(URI("ingen:/engine"), load_properties());
		_run_load.changed = false;
	}

	return !_quit_flag;
}

void
Engine::set_driver(const SPtr<Driver>& driver)
{
	_driver = driver;
	for (const auto& ctx : _run_contexts) {
		ctx->set_priority(driver->real_time_priority());
		ctx->set_rate(driver->sample_rate());
	}

	_buffer_factory->set_block_length(driver->block_length());
	_options->set(sample_rate(),
	              block_length(),
	              buffer_factory()->default_size(_world.uris().atom_Sequence));
}

SampleCount
Engine::event_time()
{
	if (ThreadManager::single_threaded) {
		return 0;
	}

	return _driver->frame_time() + _driver->block_length();
}

uint64_t
Engine::current_time() const
{
	return _clock.now_microseconds();
}

void
Engine::reset_load()
{
	_reset_load_flag = true;
}

void
Engine::init(double sample_rate, uint32_t block_length, size_t seq_size)
{
	set_driver(SPtr<Driver>(new DirectDriver(*this, sample_rate, block_length, seq_size)));
}

bool
Engine::supports_dynamic_ports() const
{
	return !_driver || _driver->dynamic_ports();
}

bool
Engine::activate()
{
	if (!_driver) {
		return false;
	}

	ThreadManager::single_threaded = true;

	const ingen::URIs& uris = _world.uris();

	if (!_root_graph) {
		// No root graph has been loaded, create an empty one
		const Properties properties = {
			{uris.rdf_type, uris.ingen_Graph},
			{uris.ingen_polyphony,
			 Property(_world.forge().make(1),
			          Resource::Graph::INTERNAL)}};

		enqueue_event(
			new events::CreateGraph(
				*this, SPtr<Interface>(), -1, 0, Raul::Path("/"), properties));

		flush_events(std::chrono::milliseconds(10));
		if (!_root_graph) {
			return false;
		}
	}

	_driver->activate();
	_root_graph->enable();

	ThreadManager::single_threaded = false;
	_activated = true;

	return true;
}

void
Engine::deactivate()
{
	if (_driver) {
		_driver->deactivate();
	}

	if (_root_graph) {
		_root_graph->deactivate();
	}

	ThreadManager::single_threaded = true;
	_activated = false;
}

unsigned
Engine::run(uint32_t sample_count)
{
	RunContext& ctx = run_context();
	_cycle_start_time = current_time();

	post_processor()->set_end_time(ctx.end());

	// Process events that came in during the last cycle
	// (Aiming for jitter-free 1 block event latency, ideally)
	const unsigned n_processed_events = process_events();

	// Reset load if graph structure has changed
	if (_reset_load_flag) {
		_run_load        = Load();
		_reset_load_flag = false;
	}

	// Run root graph
	if (_root_graph) {
		// Apply control bindings to input
		control_bindings()->pre_process(
			ctx, _root_graph->port_impl(0)->buffer(0).get());

		// Run root graph for this cycle
		_root_graph->process(ctx);

		// Emit control binding feedback
		control_bindings()->post_process(
			ctx, _root_graph->port_impl(1)->buffer(0).get());
	}

	// Update load for this cycle
	if (ctx.duration() > 0) {
		_run_load.update(current_time() - _cycle_start_time, ctx.duration());
	}

	return n_processed_events;
}

bool
Engine::pending_events() const
{
	return !_pre_processor->empty() || _post_processor->pending();
}

void
Engine::enqueue_event(Event* ev, Event::Mode mode)
{
	_pre_processor->event(ev, mode);
}

unsigned
Engine::process_events()
{
	const size_t MAX_EVENTS_PER_CYCLE = run_context().nframes() / 8;
	return _pre_processor->process(
		run_context(), *_post_processor, MAX_EVENTS_PER_CYCLE);
}

unsigned
Engine::process_all_events()
{
	return _pre_processor->process(run_context(), *_post_processor, 0);
}

Log&
Engine::log() const
{
	return _world.log();
}

void
Engine::register_client(const SPtr<Interface>& client)
{
	log().info("Registering client <%1%>\n", client->uri().c_str());
	_broadcaster->register_client(client);
}

bool
Engine::unregister_client(const SPtr<Interface>& client)
{
	log().info("Unregistering client <%1%>\n", client->uri().c_str());
	return _broadcaster->unregister_client(client);
}

} // namespace server
} // namespace ingen
