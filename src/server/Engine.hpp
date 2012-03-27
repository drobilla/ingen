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


#ifndef INGEN_ENGINE_ENGINE_HPP
#define INGEN_ENGINE_ENGINE_HPP

#include <set>
#include <vector>

#include <boost/utility.hpp>

#include "raul/SharedPtr.hpp"

#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"

namespace Raul { class Maid; }

namespace Ingen {

namespace Shared { class World; }

namespace Server {

class BufferFactory;
class ClientBroadcaster;
class ControlBindings;
class Driver;
class EngineStore;
class EventSource;
class MessageContext;
class NodeFactory;
class PostProcessor;
class ProcessContext;

/**
   The engine which executes the process graph.

   This is a simple class that provides pointers to the various components
   that make up the engine implementation.  In processes with a local engine,
   it can be accessed via the Ingen::Shared::World.

   @ingroup engine
*/
class Engine : public boost::noncopyable, public EngineBase
{
public:
	explicit Engine(Ingen::Shared::World* world);

	virtual ~Engine();

	virtual bool activate();

	virtual void deactivate();

	virtual void quit();

	virtual bool main_iteration();

	virtual void set_driver(SharedPtr<Driver> driver);

	virtual void add_event_source(SharedPtr<EventSource> source);

	virtual void process_events(ProcessContext& context);

	virtual void register_client(const Raul::URI& uri, Interface* client);

	virtual bool unregister_client(const Raul::URI& uri);

	Ingen::Shared::World* world() const { return _world; }

	ClientBroadcaster*    broadcaster()      const { return _broadcaster; }
	BufferFactory*        buffer_factory()   const { return _buffer_factory; }
	ControlBindings*      control_bindings() const { return _control_bindings; }
	Driver*               driver()           const { return _driver.get(); }
	Raul::Maid*           maid()             const { return _maid; }
	MessageContext*       message_context()  const { return _message_context; }
	NodeFactory*          node_factory()     const { return _node_factory; }
	PostProcessor*        post_processor()   const { return _post_processor; }

	SharedPtr<EngineStore> engine_store() const;

	size_t event_queue_size() const;

private:
	Ingen::Shared::World* _world;

	ClientBroadcaster* _broadcaster;
	BufferFactory*     _buffer_factory;
	ControlBindings*   _control_bindings;
	SharedPtr<Driver>  _driver;
	Raul::Maid*        _maid;
	MessageContext*    _message_context;
	NodeFactory*       _node_factory;
	PostProcessor*     _post_processor;

	typedef std::set< SharedPtr<EventSource> > EventSources;
	EventSources _event_sources;

	bool _quit_flag;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_ENGINE_HPP
