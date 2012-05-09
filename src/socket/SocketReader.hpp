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

#ifndef INGEN_SOCKET_SOCKET_READER_HPP
#define INGEN_SOCKET_SOCKET_READER_HPP

#include "raul/Thread.hpp"
#include "sord/sord.h"

namespace Ingen {

namespace Shared {
class World;
class Interface;
}

namespace Socket {

class SocketReader : public Raul::Thread
{
public:
	SocketReader(Shared::World& world, Interface& iface, int conn);
	~SocketReader();

private:
	virtual void _run();

	static SerdStatus set_base_uri(SocketReader*   iface,
	                               const SerdNode* uri_node);

	static SerdStatus set_prefix(SocketReader*   iface,
	                             const SerdNode* name,
	                             const SerdNode* uri_node);

	static SerdStatus write_statement(SocketReader*      iface,
	                                  SerdStatementFlags flags,
	                                  const SerdNode*    graph,
	                                  const SerdNode*    subject,
	                                  const SerdNode*    predicate,
	                                  const SerdNode*    object,
	                                  const SerdNode*    object_datatype,
	                                  const SerdNode*    object_lang);

	Shared::World& _world;
	Interface&     _iface;
	SerdEnv*       _env;
	SordInserter*  _inserter;
	SordNode*      _msg_node;
	int            _conn;
};

}  // namespace Ingen
}  // namespace Socket

#endif  // INGEN_SOCKET_SOCKET_READER_HPP
