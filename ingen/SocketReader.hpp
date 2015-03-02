/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_SOCKET_READER_HPP
#define INGEN_SOCKET_READER_HPP

#include <thread>

#include "ingen/ingen.h"
#include "raul/Socket.hpp"
#include "sord/sord.h"

namespace Ingen {

class Interface;
class World;

/** Calls Interface methods based on Turtle messages received via socket. */
class INGEN_API SocketReader
{
public:
	SocketReader(World&             world,
	             Interface&         iface,
	             SPtr<Raul::Socket> sock);

	~SocketReader();

private:
	void run();

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

	World&             _world;
	Interface&         _iface;
	SerdEnv*           _env;
	SordInserter*      _inserter;
	SordNode*          _msg_node;
	SPtr<Raul::Socket> _socket;
	bool               _exit_flag;
	std::thread        _thread;
};

}  // namespace Ingen

#endif  // INGEN_SOCKET_READER_HPP
