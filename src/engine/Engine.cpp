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
#include <iostream>
#include <unistd.h>
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
#include "AudioDriver.hpp"
#include "BufferFactory.hpp"
#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "Event.hpp"
#include "MessageContext.hpp"
#include "MidiDriver.hpp"
#include "NodeFactory.hpp"
#include "OSCDriver.hpp"
#include "PatchImpl.hpp"
#include "PostProcessor.hpp"
#include "PostProcessor.hpp"
#include "ProcessContext.hpp"
#include "ProcessSlave.hpp"
#include "QueuedEngineInterface.hpp"
#include "QueuedEventSource.hpp"
#include "ThreadManager.hpp"
#include "tuning.hpp"

using namespace std;

namespace Ingen {

using namespace Shared;


Engine::Engine(Ingen::Shared::World* world)
	: _world(world)
	, _midi_driver(NULL)
	, _osc_driver(NULL)
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
	delete _osc_driver;
	delete _post_processor;

	delete _maid;

	munlockall();
}


SharedPtr<EngineStore>
Engine::engine_store() const
{
	 return PtrCast<EngineStore>(_world->store);
}


Driver*
Engine::driver(PortType type, EventType event_type)
{
	if (type == PortType::AUDIO) {
		return _audio_driver.get();
	} else if (type == PortType::EVENTS) {
		if (event_type == EventType::MIDI) {
			return _midi_driver;
		} else if (event_type == EventType::OSC) {
			return _osc_driver;
		}
	}

	return NULL;
}


void
Engine::set_driver(PortType type, SharedPtr<Driver> driver)
{
	if (type == PortType::AUDIO) {
		_audio_driver = PtrCast<AudioDriver>(driver);
	} else {
		cerr << "WARNING: Unable to set driver for type " << type.uri() << endl;
	}
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
	cout << "[Main] Done main loop." << endl;

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


void
Engine::set_midi_driver(MidiDriver* driver)
{
	_midi_driver = driver;
}


bool
Engine::activate()
{
	assert(_audio_driver);

	_message_context->Thread::start();

	uint32_t parallelism = _world->conf->option("parallelism").get_int32();

	if (!_midi_driver)
		_midi_driver = new DummyMidiDriver();

	for (EventSources::iterator i = _event_sources.begin(); i != _event_sources.end(); ++i)
		(*i)->activate_source();

	// Create root patch
	PatchImpl* root_patch = _audio_driver->root_patch();
	if (!root_patch) {
		root_patch = new PatchImpl(*this, "", 1, NULL,
				_audio_driver->sample_rate(), _audio_driver->buffer_size(), 1);
		root_patch->meta().set_property("rdf:type", Raul::Atom(Raul::Atom::URI, "ingen:Patch"));
		root_patch->meta().set_property("ingen:polyphony", Raul::Atom(int32_t(1)));
		root_patch->set_property("rdf:type", Raul::Atom(Raul::Atom::URI, "ingen:Node"));
		root_patch->activate();
		_world->store->add(root_patch);
		root_patch->compiled_patch(root_patch->compile());
		_audio_driver->set_root_patch(root_patch);
	}

	_audio_driver->activate();
	_midi_driver->attach(*_audio_driver.get());

	_process_slaves.clear();
	_process_slaves.reserve(parallelism);
	for (size_t i=0; i < parallelism - 1; ++i)
		_process_slaves.push_back(new ProcessSlave(*this, _audio_driver->is_realtime()));

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

	/*for (Tree<GraphObject*>::iterator i = _engine_store->objects().begin();
			i != _engine_store->objects().end(); ++i)
		if ((*i)->as_node() != NULL && (*i)->as_node()->parent() == NULL)
			(*i)->as_node()->deactivate();*/

	if (_midi_driver)
		_midi_driver->deactivate();

	_audio_driver->deactivate();

	_audio_driver->root_patch()->deactivate();

	/*for (size_t i=0; i < _process_slaves.size(); ++i) {
		delete _process_slaves[i];
	}*/

	//_process_slaves.clear();

	// Finalize any lingering events (unlikely)
	//_post_processor->process();

	//_audio_driver.reset();
	//_event_sources.clear();

	_activated = false;
}


void
Engine::process_events(ProcessContext& context)
{
	for (EventSources::iterator i = _event_sources.begin(); i != _event_sources.end(); ++i)
		(*i)->process(*_post_processor, context);
}


} // namespace Ingen
