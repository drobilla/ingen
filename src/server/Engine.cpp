/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <sys/mman.h>
#include <unistd.h>

#include "events/CreatePort.hpp"
#include "ingen/shared/Configuration.hpp"
#include "ingen/shared/LV2Features.hpp"
#include "ingen/shared/Store.hpp"
#include "ingen/shared/URIs.hpp"
#include "ingen/shared/World.hpp"
#include "raul/Deletable.hpp"
#include "raul/Maid.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/log.hpp"

#include "BufferFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "Event.hpp"
#include "EventWriter.hpp"
#include "MessageContext.hpp"
#include "NodeFactory.hpp"
#include "PatchImpl.hpp"
#include "PostProcessor.hpp"
#include "PreProcessor.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

bool ThreadManager::single_threaded = true;

Engine::Engine(Ingen::Shared::World* a_world)
	: _world(a_world)
	, _broadcaster(new ClientBroadcaster())
	, _control_bindings(NULL)
	, _maid(new Raul::Maid(event_queue_size()))
	, _message_context(new MessageContext(*this))
	, _node_factory(new NodeFactory(a_world))
	, _pre_processor(new PreProcessor())
	, _post_processor(new PostProcessor(*this))
	, _event_writer(new EventWriter(*this))
	, _process_context(*this)
	, _quit_flag(false)
{
	if (a_world->store()) {
		SharedPtr<EngineStore> estore = PtrCast<EngineStore>(a_world->store());
		_buffer_factory = estore->buffer_factory().get();
	} else {
		_buffer_factory = new BufferFactory(*this, a_world->uris());
		a_world->set_store(
			SharedPtr<Ingen::Shared::Store>(
				new EngineStore(SharedPtr<BufferFactory>(_buffer_factory))));
	}

	_control_bindings = new ControlBindings(*this);
}

Engine::~Engine()
{
	deactivate();

	SharedPtr<EngineStore> store = engine_store();
	if (store)
		for (EngineStore::iterator i = store->begin(); i != store->end(); ++i)
			if ( ! PtrCast<GraphObjectImpl>(i->second)->parent() )
				i->second.reset();

	delete _maid;
	delete _post_processor;
	delete _node_factory;
	delete _message_context;
	delete _control_bindings;
	delete _broadcaster;

	munlockall();
}

SharedPtr<EngineStore>
Engine::engine_store() const
{
	return PtrCast<EngineStore>(_world->store());
}

size_t
Engine::event_queue_size() const
{
	return world()->conf().option("queue-size").get_int();
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
Engine::set_driver(SharedPtr<Driver> driver)
{
	_driver = driver;
}

static void
execute_and_delete_event(ProcessContext& context, Event* ev)
{
	ev->pre_process();
	ev->execute(context);
	ev->post_process();
	delete ev;
}

bool
Engine::activate()
{
	assert(_driver);
	ThreadManager::single_threaded = true;

	_buffer_factory->set_block_length(_driver->block_length());

	_message_context->Thread::start();

	const Ingen::Shared::URIs& uris  = world()->uris();
	Ingen::Forge&              forge = world()->forge();

	// Create root patch
	if (!_root_patch) {
		_root_patch = new PatchImpl(
			*this, "root", 1, NULL, _driver->sample_rate(), 1);
		_root_patch->set_property(
			uris.rdf_type,
			Resource::Property(uris.ingen_Patch, Resource::INTERNAL));
		_root_patch->set_property(
			uris.ingen_polyphony,
			Resource::Property(_world->forge().make(int32_t(1)),
			                   Resource::INTERNAL));
		_root_patch->activate(*_buffer_factory);
		_world->store()->add(_root_patch);
		_root_patch->compiled_patch(_root_patch->compile());

		ProcessContext context(*this);

		Resource::Properties control_properties;
		control_properties.insert(make_pair(uris.lv2_name,
		                                    forge.alloc("Control")));
		control_properties.insert(make_pair(uris.rdf_type,
		                                    uris.atom_AtomPort));
		control_properties.insert(make_pair(uris.atom_bufferType,
		                                    uris.atom_Sequence));

		// Add control input
		Resource::Properties in_properties(control_properties);
		in_properties.insert(make_pair(uris.rdf_type, uris.lv2_InputPort));
		in_properties.insert(make_pair(uris.lv2_index, forge.make(0)));
		in_properties.insert(make_pair(uris.lv2_portProperty,
		                               uris.lv2_connectionOptional));
		in_properties.insert(
			make_pair(uris.ingen_canvasX,
			          Resource::Property(forge.make(32.0f), Resource::EXTERNAL)));
		in_properties.insert(
			make_pair(uris.ingen_canvasY,
			          Resource::Property(forge.make(32.0f), Resource::EXTERNAL)));

		execute_and_delete_event(
			context, new Events::CreatePort(*this, NULL, -1, 0,
			                                "/control_in", false, in_properties));

		// Add control out
		Resource::Properties out_properties(control_properties);
		out_properties.insert(make_pair(uris.rdf_type, uris.lv2_OutputPort));
		out_properties.insert(make_pair(uris.lv2_index, forge.make(1)));
		in_properties.insert(make_pair(uris.lv2_portProperty,
		                               uris.lv2_connectionOptional));
		out_properties.insert(
			make_pair(uris.ingen_canvasX,
			          Resource::Property(forge.make(128.0f), Resource::EXTERNAL)));
		out_properties.insert(
			make_pair(uris.ingen_canvasY,
			          Resource::Property(forge.make(32.0f), Resource::EXTERNAL)));

		execute_and_delete_event(
			context, new Events::CreatePort(*this, NULL, -1, 0,
			                                "/control_out", true, out_properties));
	}

	_driver->activate();
	_root_patch->enable();

	ThreadManager::single_threaded = false;

	return true;
}

void
Engine::deactivate()
{
	_driver->deactivate();
	_root_patch->deactivate();

	ThreadManager::single_threaded = true;
}

void
Engine::run(uint32_t sample_count)
{
	// Apply control bindings to input
	control_bindings()->pre_process(
		_process_context, _root_patch->port_impl(0)->buffer(0).get());

	post_processor()->set_end_time(_process_context.end());

	// Process events that came in during the last cycle
	// (Aiming for jitter-free 1 block event latency, ideally)
	process_events(_process_context);

	// Run root patch
	if (_root_patch) {
		_root_patch->process(_process_context);
	}

	// Emit control binding feedback
	control_bindings()->post_process(
		_process_context, _root_patch->port_impl(1)->buffer(0).get());

	// Signal message context to run if necessary
	if (message_context()->has_requests()) {
		message_context()->signal(_process_context);
	}
}

bool
Engine::pending_events()
{
	return !_pre_processor->empty();
}

void
Engine::enqueue_event(Event* ev)
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
	_pre_processor->event(ev);
}

void
Engine::process_events(ProcessContext& context)
{
	ThreadManager::assert_thread(THREAD_PROCESS);
	_pre_processor->process(*_post_processor, context);
}

void
Engine::register_client(const Raul::URI& uri, SharedPtr<Interface> client)
{
	_broadcaster->register_client(uri, client);
}

bool
Engine::unregister_client(const Raul::URI& uri)
{
	return _broadcaster->unregister_client(uri);
}

} // namespace Server
} // namespace Ingen
