/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"

#include "events/CreateGraph.hpp"
#include "ingen/AtomReader.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Log.hpp"
#include "ingen/Store.hpp"
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

Raul::ThreadVar<unsigned> ThreadManager::flags(0);
bool                      ThreadManager::single_threaded(true);

Engine::Engine(Ingen::World* world)
	: _world(world)
	, _block_factory(new BlockFactory(world))
	, _broadcaster(new Broadcaster())
	, _buffer_factory(new BufferFactory(*this, world->uris()))
	, _control_bindings(NULL)
	, _event_writer(new EventWriter(*this))
	, _atom_interface(new AtomReader(world->uri_map(), world->uris(), world->log(), *_event_writer))
	, _maid(new Raul::Maid())
	, _options(new LV2Options(world->uris()))
	, _undo_stack(new UndoStack(_world->uris(), _world->uri_map()))
	, _redo_stack(new UndoStack(_world->uris(), _world->uri_map()))
	, _pre_processor(new PreProcessor(*this))
	, _post_processor(new PostProcessor(*this))
	, _root_graph(NULL)
	, _worker(new Worker(world->log(), event_queue_size()))
	, _sync_worker(new Worker(world->log(), event_queue_size(), true))
	, _listener(NULL)
	, _run_context(*this)
	, _rand_engine(0)
	, _uniform_dist(0.0f, 1.0f)
	, _quit_flag(false)
	, _direct_driver(true)
{
	if (!world->store()) {
		world->set_store(SPtr<Ingen::Store>(new Store()));
	}

	_control_bindings = new ControlBindings(*this);

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
}

Engine::~Engine()
{
	_root_graph = NULL;
	deactivate();

	// Process all pending events
	const FrameTime end = std::numeric_limits<FrameTime>::max();
	_run_context.locate(_run_context.end(), end - _run_context.end());
	_post_processor->set_end_time(end);
	_post_processor->process();
	while (!_pre_processor->empty()) {
		_pre_processor->process(_run_context, *_post_processor, 1);
		_post_processor->process();
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
	delete _block_factory;
	delete _control_bindings;
	delete _broadcaster;
	delete _event_writer;
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

SPtr<Store>
Engine::store() const
{
	return _world->store();
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

bool
Engine::main_iteration()
{
	_post_processor->process();
	_maid->cleanup();
	return !_quit_flag;
}

void
Engine::set_driver(SPtr<Driver> driver)
{
	_driver = driver;
}

SampleCount
Engine::event_time()
{
	if (ThreadManager::single_threaded) {
		return 0;
	}

	const SampleCount start = _direct_driver
		? _run_context.start()
		: _driver->frame_time();

	/* Exactly one cycle latency (some could run ASAP if we get lucky, but not
	   always, and a slight constant latency is far better than jittery lower
	   (average) latency */
	return start + _driver->block_length();
}

void
Engine::init(double sample_rate, uint32_t block_length, size_t seq_size)
{
	set_driver(SPtr<Driver>(new DirectDriver(sample_rate, block_length, seq_size)));
	_direct_driver = true;
}

bool
Engine::activate()
{
	if (!_driver) {
		return false;
	}

	ThreadManager::single_threaded = true;

	_buffer_factory->set_block_length(_driver->block_length());
	_options->set(driver()->sample_rate(),
	              driver()->block_length(),
	              buffer_factory()->default_size(_world->uris().atom_Sequence));

	const Ingen::URIs& uris = world()->uris();

	if (!_root_graph) {
		// Create root graph
		Resource::Properties graph_properties;
		graph_properties.insert(
			make_pair(uris.rdf_type,
			          Resource::Property(uris.ingen_Graph)));
		graph_properties.insert(
			make_pair(uris.ingen_polyphony,
			          Resource::Property(_world->forge().make(1),
			                             Resource::Graph::INTERNAL)));

		Events::CreateGraph ev(
			*this, SPtr<Interface>(), -1, 0, Raul::Path("/"), graph_properties);

		// Execute in "fake" process context (we are single threaded)
		RunContext context(*this);
		ev.pre_process();
		ev.execute(context);
		ev.post_process();

		_root_graph = ev.graph();
	}

	_driver->activate();
	_root_graph->enable();

	ThreadManager::single_threaded = false;

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
}

unsigned
Engine::run(uint32_t sample_count)
{
	_run_context.locate(_run_context.end(), sample_count);

	// Apply control bindings to input
	control_bindings()->pre_process(
		_run_context, _root_graph->port_impl(0)->buffer(0).get());

	post_processor()->set_end_time(_run_context.end());

	// Process events that came in during the last cycle
	// (Aiming for jitter-free 1 block event latency, ideally)
	const unsigned n_processed_events = process_events();

	// Run root graph
	if (_root_graph) {
		_root_graph->process(_run_context);

		// Emit control binding feedback
		control_bindings()->post_process(
			_run_context, _root_graph->port_impl(1)->buffer(0).get());
	}

	return n_processed_events;
}

bool
Engine::pending_events()
{
	return !_pre_processor->empty();
}

void
Engine::enqueue_event(Event* ev, Event::Mode mode)
{
	_pre_processor->event(ev, mode);
}

unsigned
Engine::process_events()
{
	const size_t MAX_EVENTS_PER_CYCLE = _run_context.nframes() / 8;
	return _pre_processor->process(
		_run_context, *_post_processor, MAX_EVENTS_PER_CYCLE);
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
