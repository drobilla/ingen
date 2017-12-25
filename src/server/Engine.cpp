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

#include "ingen_config.h"

#include <sys/mman.h>

#include <limits>
#include <thread>

#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"

#include "events/CreateGraph.hpp"
#include "ingen/AtomReader.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Log.hpp"
#include "ingen/Store.hpp"
#include "ingen/StreamWriter.hpp"
#include "ingen/Tee.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "raul/Maid.hpp"

#include "BlockFactory.hpp"
#include "Broadcaster.hpp"
#include "BufferFactory.hpp"
#include "ControlBindings.hpp"
#include "DirectDriver.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "EventWriter.hpp"
#include "GraphImpl.hpp"
#include "LV2Options.hpp"
#include "PostProcessor.hpp"
#include "PreProcessContext.hpp"
#include "PreProcessor.hpp"
#include "RunContext.hpp"
#include "ThreadManager.hpp"
#include "UndoStack.hpp"
#include "Worker.hpp"
#ifdef HAVE_SOCKET
#include "SocketListener.hpp"
#endif

using namespace std;

namespace Ingen {
namespace Server {

INGEN_THREAD_LOCAL unsigned ThreadManager::flags(0);
bool               ThreadManager::single_threaded(true);

Engine::Engine(Ingen::World* world)
	: _world(world)
	, _maid(new Raul::Maid)
	, _block_factory(new BlockFactory(world))
	, _broadcaster(new Broadcaster())
	, _buffer_factory(new BufferFactory(*this, world->uris()))
	, _control_bindings(nullptr)
	, _event_writer(new EventWriter(*this))
	, _interface(_event_writer)
	, _atom_interface(nullptr)
	, _options(new LV2Options(world->uris()))
	, _undo_stack(new UndoStack(_world->uris(), _world->uri_map()))
	, _redo_stack(new UndoStack(_world->uris(), _world->uri_map()))
	, _pre_processor(new PreProcessor(*this))
	, _post_processor(new PostProcessor(*this))
	, _root_graph(nullptr)
	, _worker(new Worker(world->log(), event_queue_size()))
	, _sync_worker(new Worker(world->log(), event_queue_size(), true))
	, _listener(nullptr)
	, _cycle_start_time(0)
	, _rand_engine(0)
	, _uniform_dist(0.0f, 1.0f)
	, _quit_flag(false)
	, _reset_load_flag(false)
	, _atomic_bundles(world->conf().option("atomic-bundles").get<int32_t>())
	, _activated(false)
{
	if (!world->store()) {
		world->set_store(SPtr<Ingen::Store>(new Store()));
	}

	_control_bindings = new ControlBindings(*this);

	for (int i = 0; i < world->conf().option("threads").get<int32_t>(); ++i) {
		Raul::RingBuffer* ring = new Raul::RingBuffer(24 * event_queue_size());
		_notifications.push_back(ring);
		_run_contexts.push_back(new RunContext(*this, ring, i, i > 0));
	}

	_world->lv2_features().add_feature(_worker->schedule_feature());
	_world->lv2_features().add_feature(_options);
	_world->lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_BUF_SIZE__powerOf2BlockLength)));
	_world->lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_BUF_SIZE__fixedBlockLength)));
	_world->lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_BUF_SIZE__boundedBlockLength)));
	_world->lv2_features().add_feature(
		SPtr<LV2Features::Feature>(
			new LV2Features::EmptyFeature(LV2_STATE__loadDefaultState)));

	if (world->conf().option("dump").get<int32_t>()) {
		_interface = std::make_shared<Tee>(
			Tee::Sinks{
				_event_writer,
				std::make_shared<StreamWriter>(world->uri_map(),
				                               world->uris(),
				                               Raul::URI("ingen:/engine"),
				                               stderr,
				                               ColorContext::Color::MAGENTA)});
	}

	_atom_interface = new AtomReader(
		world->uri_map(), world->uris(), world->log(), *_interface.get());
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

	delete _atom_interface;

	// Delete run contexts
	_quit_flag = true;
	_tasks_available.notify_all();
	for (RunContext* ctx : _run_contexts) {
		ctx->join();
		delete ctx;
	}
	for (Raul::RingBuffer* ring : _notifications) {
		delete ring;
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

	_world->set_store(SPtr<Ingen::Store>());

#ifdef HAVE_SOCKET
	delete _listener;
#endif
	delete _pre_processor;
	delete _post_processor;
	delete _undo_stack;
	delete _redo_stack;
	delete _block_factory;
	delete _control_bindings;
	delete _broadcaster;
	delete _sync_worker;
	delete _worker;
	delete _maid;

	_driver.reset();

	delete _buffer_factory;

	munlockall();
}

void
Engine::listen()
{
#ifdef HAVE_SOCKET
	_listener = new SocketListener(*this);
#endif
}

void
Engine::advance(SampleCount nframes)
{
	for (RunContext* ctx : _run_contexts) {
		ctx->locate(ctx->start() + nframes, block_length());
	}
}

void
Engine::locate(FrameTime s, SampleCount nframes)
{
	for (RunContext* ctx : _run_contexts) {
		ctx->locate(s, nframes);
	}
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
	for (RunContext* ctx : _run_contexts) {
		ctx->emit_notifications(end);
	}
}

bool
Engine::pending_notifications()
{
	for (const RunContext* ctx : _run_contexts) {
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
		const unsigned    id  = (start_thread + i) % _run_contexts.size();
		RunContext* const ctx = _run_contexts[id];
		Task*             par = ctx->task();
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
	return _world->store();
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
	return world()->conf().option("queue-size").get<int32_t>();
}

void
Engine::quit()
{
	_quit_flag = true;
}

Properties
Engine::load_properties() const
{
	const Ingen::URIs& uris = world()->uris();

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
		_broadcaster->put(Raul::URI("ingen:/engine"), load_properties());
		_run_load.changed = false;
	}

	return !_quit_flag;
}

void
Engine::set_driver(SPtr<Driver> driver)
{
	_driver = driver;
	for (RunContext* ctx : _run_contexts) {
		ctx->set_priority(driver->real_time_priority());
		ctx->set_rate(driver->sample_rate());
	}

	_buffer_factory->set_block_length(driver->block_length());
	_options->set(sample_rate(),
	              block_length(),
	              buffer_factory()->default_size(_world->uris().atom_Sequence));
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

	const Ingen::URIs& uris = world()->uris();

	if (!_root_graph) {
		// No root graph has been loaded, create an empty one
		const Properties properties = {
			{uris.rdf_type, uris.ingen_Graph},
			{uris.ingen_polyphony,
			 Property(_world->forge().make(1),
			          Resource::Graph::INTERNAL)}};

		enqueue_event(
			new Events::CreateGraph(
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

void
Engine::register_client(SPtr<Interface> client)
{
	log().info(fmt("Registering client <%1%>\n") % client->uri().c_str());
	_broadcaster->register_client(client);
}

bool
Engine::unregister_client(SPtr<Interface> client)
{
	log().info(fmt("Unregistering client <%1%>\n") % client->uri().c_str());
	return _broadcaster->unregister_client(client);
}

} // namespace Server
} // namespace Ingen
