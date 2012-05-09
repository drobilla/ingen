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

#include <string>

#include "ingen/Interface.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"
#include "sord/sord.h"

#include "../server/EventSink.hpp"
#include "../server/EventSource.hpp"
#include "../server/EventWriter.hpp"

namespace Ingen {

namespace Shared { class World; }

namespace Socket {

class SocketInterface : public Raul::Thread
                      , public Server::EventSink
                      , public Server::EventSource
{
public:
	SocketInterface(Shared::World& world, int conn);

	void event(Server::Event* ev);

	bool process(Server::PostProcessor&  dest,
	             Server::ProcessContext& context,
	             bool                    limit = true);

	~SocketInterface();

	SordInserter* inserter() { return _inserter; }

private:
	virtual void _run();

	static SerdStatus set_base_uri(SocketInterface* iface,
	                               const SerdNode*  uri_node);

	static SerdStatus set_prefix(SocketInterface* iface,
	                             const SerdNode*  name,
	                             const SerdNode*  uri_node);

	static SerdStatus write_statement(SocketInterface*   iface,
	                                  SerdStatementFlags flags,
	                                  const SerdNode*    graph,
	                                  const SerdNode*    subject,
	                                  const SerdNode*    predicate,
	                                  const SerdNode*    object,
	                                  const SerdNode*    object_datatype,
	                                  const SerdNode*    object_lang);

	Shared::World&      _world;
	Server::EventWriter _iface;
	SerdEnv*            _env;
	SordInserter*       _inserter;
	SordNode*           _msg_node;
	Server::Event*      _event;
	int                 _conn;
};

}  // namespace Ingen
}  // namespace Socket
