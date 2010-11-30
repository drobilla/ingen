/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cassert>
#include <sys/mman.h>
#include <unistd.h>
#include "raul/log.hpp"
#include "raul/Deletable.hpp"
#include "raul/Maid.hpp"
#include "raul/SharedPtr.hpp"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "common/interface/EventType.hpp"
#include "events/CreatePatch.hpp"
#include "events/CreatePort.hpp"
#include "module/World.hpp"
#include "shared/LV2Features.hpp"
#include "shared/LV2URIMap.hpp"
#include "shared/Store.hpp"
#include "BufferFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "Event.hpp"
#include "EventSource.hpp"
#include "MessageContext.hpp"
#include "NodeFactory.hpp"
#include "PatchImpl.hpp"
#include "PostProcessor.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "ProcessSlave.hpp"
#include "QueuedEngineInterface.hpp"
#include "ThreadManager.hpp"
#include "tuning.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;

bool ThreadManager::single_threaded = true;

Engine::Engine(Ingen::Shared::World* a_world)
	: _world(a_world)
	, _maid(new Raul::Maid(maid_queue_size))
	, _post_processor(new PostProcessor(*this, post_processor_queue_size))
	, _broadcaster(new ClientBroadcaster())
	, _node_factory(new NodeFactory(a_world))
	, _message_context(new MessageContext(*this))
	, _buffer_factory(new BufferFactory(*this, a_world->uris()))
	//, _buffer_factory(NULL)
	, _control_bindings(new ControlBindings(*this))
	, _quit_flag(false)
	, _activated(false)
{
	if (a_world->store()) {
		assert(PtrCast<EngineStore>(a_world->store()));
	} else {
		a_world->set_store(SharedPtr<Shared::Store>(new EngineStore()));
	}
}


Engine::~Engine()
{
	deactivate();

	SharedPtr<EngineStore> store = engine_store();
	if (store)
		for (EngineStore::iterator i = store->begin(); i != store->end(); ++i)
			if ( ! PtrCast<GraphObjectImpl>(i->second)->parent() )
				i->second.reset();

	delete _broadcaster;
	delete _node_factory;
	delete _post_processor;

	delete _maid;

	munlockall();
}


SharedPtr<EngineStore>
Engine::engine_store() const
{
	 return PtrCast<EngineStore>(_world->store());
}


int
Engine::main()
{
	Raul::Thread::get().set_context(THREAD_POST_PROCESS);

	// Loop until quit flag is set (by OSCReceiver)
	while ( ! _quit_flag) {
		nanosleep(&main_rate, NULL);
		main_iteration();
	}
	info << "Finished main loop" << endl;

	deactivate();

	return 0;
}


/** Run one iteration of the main loop.
 *
 * NOT realtime safe (this is where deletion actually occurs)
 */
bool
Engine::main_iteration()
{
	_post_processor->process();
	_maid->cleanup();

	return !_quit_flag;
}


void
Engine::add_event_source(SharedPtr<EventSource> source)
{
	_event_sources.insert(source);
}


static void
execute_and_delete_event(ProcessContext& context, QueuedEvent* ev)
{
	ev->pre_process();
	ev->execute(context);
	ev->post_process();
	delete ev;
}


bool
Engine::activate()
{
	if (_activated)
		return true;

	assert(_driver);
	//assert(ThreadManager::single_threaded == true);
	ThreadManager::single_threaded = true;

	_buffer_factory->set_block_length(_driver->block_length());

	_message_context->Thread::start();

	uint32_t parallelism = _world->conf()->option("parallelism").get_int32();

	for (EventSources::iterator i = _event_sources.begin(); i != _event_sources.end(); ++i)
		(*i)->activate_source();

	const LV2URIMap& uris = *world()->uris().get();

	// Create root patch
	PatchImpl* root_patch = _driver->root_patch();
	if (!root_patch) {
		root_patch = new PatchImpl(*this, "root", 1, NULL, _driver->sample_rate(), 1);
		root_patch->meta().set_property(uris.rdf_type, uris.ingen_Patch);
		root_patch->meta().set_property(uris.ingen_polyphony, Raul::Atom(int32_t(1)));
		root_patch->set_property(uris.rdf_type, uris.ingen_Node);
		root_patch->activate(*_buffer_factory);
		_world->store()->add(root_patch);
		root_patch->compiled_patch(root_patch->compile());
		_driver->set_root_patch(root_patch);

		ProcessContext context(*this);

		Shared::Resource::Properties control_properties;
		control_properties.insert(make_pair(uris.lv2_name, "Control"));
		control_properties.insert(make_pair(uris.rdf_type, uris.ev_EventPort));

		// Add control input
		Shared::Resource::Properties in_properties(control_properties);
		in_properties.insert(make_pair(uris.rdf_type, uris.lv2_InputPort));
		in_properties.insert(make_pair(uris.lv2_index, 0));
		in_properties.insert(make_pair(uris.ingenui_canvas_x, 32.0f));
		in_properties.insert(make_pair(uris.ingenui_canvas_y, 32.0f));

		execute_and_delete_event(context, new Events::CreatePort(
				*this, SharedPtr<Request>(), 0,
				"/control_in", uris.ev_EventPort, false, in_properties));

		// Add control out
		Shared::Resource::Properties out_properties(control_properties);
		out_properties.insert(make_pair(uris.rdf_type, uris.lv2_OutputPort));
		out_properties.insert(make_pair(uris.lv2_index, 1));
		out_properties.insert(make_pair(uris.ingenui_canvas_x, 128.0f));
		out_properties.insert(make_pair(uris.ingenui_canvas_y, 32.0f));

		execute_and_delete_event(context, new Events::CreatePort(
				*this, SharedPtr<Request>(), 0,
				"/control_out", uris.ev_EventPort, true, out_properties));
	}

	_driver->activate();

	_process_slaves.clear();
	_process_slaves.reserve(parallelism);
	for (size_t i=0; i < parallelism - 1; ++i)
		_process_slaves.push_back(new ProcessSlave(*this, _driver->is_realtime()));

	root_patch->enable();

	_activated = true;
	ThreadManager::single_threaded = false;

	return true;
}


void
Engine::deactivate()
{
	if (!_activated)
		return;

	for (EventSources::iterator i = _event_sources.begin(); i != _event_sources.end(); ++i)
		(*i)->deactivate_source();

	_driver->deactivate();
	_driver->root_patch()->deactivate();

	_activated = false;
	ThreadManager::single_threaded = true;
}


void
Engine::process_events(ProcessContext& context)
{
	for (EventSources::iterator i = _event_sources.begin(); i != _event_sources.end(); ++i)
		(*i)->process(*_post_processor, context);
}


} // namespace Ingen
