/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "uri-map.lv2/uri-map.h"
#include "common/interface/EventType.hpp"
#include "events/CreatePatch.hpp"
#include "module/World.hpp"
#include "shared/LV2Features.hpp"
#include "shared/LV2URIMap.hpp"
#include "shared/Store.hpp"
#include "Driver.hpp"
#include "BufferFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "Event.hpp"
#include "MessageContext.hpp"
#include "NodeFactory.hpp"
#include "PatchImpl.hpp"
#include "PostProcessor.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "ProcessSlave.hpp"
#include "QueuedEngineInterface.hpp"
#include "EventSource.hpp"
#include "ThreadManager.hpp"
#include "tuning.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


Engine::Engine(Ingen::Shared::World* world)
	: _world(world)
	, _maid(new Raul::Maid(maid_queue_size))
	, _post_processor(new PostProcessor(*this, post_processor_queue_size))
	, _broadcaster(new ClientBroadcaster())
	, _node_factory(new NodeFactory(world))
	, _message_context(new MessageContext(*this))
	, _buffer_factory(new BufferFactory(*this, PtrCast<LV2URIMap>(
			world->lv2_features->feature(LV2_URI_MAP_URI))))
	, _quit_flag(false)
	, _activated(false)
{
	if (world->store) {
		assert(PtrCast<EngineStore>(world->store));
	} else {
		world->store = SharedPtr<Shared::Store>(new EngineStore());
	}
}


Engine::~Engine()
{
	deactivate();

	for (EngineStore::iterator i = engine_store()->begin();
			i != engine_store()->end(); ++i) {
		if ( ! PtrCast<GraphObjectImpl>(i->second)->parent() )
			i->second.reset();
	}

	delete _broadcaster;
	delete _node_factory;
	delete _post_processor;

	delete _maid;

	munlockall();
}


SharedPtr<EngineStore>
Engine::engine_store() const
{
	 return PtrCast<EngineStore>(_world->store);
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


Ingen::QueuedEngineInterface*
Engine::new_local_interface()
{
	return new Ingen::QueuedEngineInterface(*this, Ingen::event_queue_size);
}


void
Engine::add_event_source(SharedPtr<EventSource> source)
{
	_event_sources.insert(source);
}


bool
Engine::activate()
{
	assert(_driver);

	_message_context->Thread::start();

	uint32_t parallelism = _world->conf->option("parallelism").get_int32();

	for (EventSources::iterator i = _event_sources.begin(); i != _event_sources.end(); ++i)
		(*i)->activate_source();

	// Create root patch
	PatchImpl* root_patch = _driver->root_patch();
	if (!root_patch) {
		root_patch = new PatchImpl(*this, "", 1, NULL,
				_driver->sample_rate(), _driver->buffer_size(), 1);
		root_patch->meta().set_property("rdf:type", Raul::Atom(Raul::Atom::URI, "ingen:Patch"));
		root_patch->meta().set_property("ingen:polyphony", Raul::Atom(int32_t(1)));
		root_patch->set_property("rdf:type", Raul::Atom(Raul::Atom::URI, "ingen:Node"));
		root_patch->activate();
		_world->store->add(root_patch);
		root_patch->compiled_patch(root_patch->compile());
		_driver->set_root_patch(root_patch);
	}

	_driver->activate();

	_process_slaves.clear();
	_process_slaves.reserve(parallelism);
	for (size_t i=0; i < parallelism - 1; ++i)
		_process_slaves.push_back(new ProcessSlave(*this, _driver->is_realtime()));

	root_patch->enable();

	//_post_processor->start();

	_activated = true;

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
}


void
Engine::process_events(ProcessContext& context)
{
	for (EventSources::iterator i = _event_sources.begin(); i != _event_sources.end(); ++i)
		(*i)->process(*_post_processor, context);
}


} // namespace Ingen
