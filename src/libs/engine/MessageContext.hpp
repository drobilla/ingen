/* This file is part of Ingen.
 * Copyright (C) 2007-2008 Dave Robillard <http://drobilla.net>
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

#ifndef MESSAGECONTEXT_H
#define MESSAGECONTEXT_H

#include "EventSink.hpp"

namespace Ingen {

class NodeImpl;

/** Context of a message_process() call.
 *
 * The message context is a non-hard-realtime thread used to execute things
 * that can take too long to execute in an audio thread, and do sloppy timed
 * event propagation and scheduling.  Interface to plugins via the
 * LV2 contexts extension.
 *
 * \ingroup engine
 */
class MessageContext
{
public:
	MessageContext(Engine& engine)
		: _engine(engine)
	{}

	void run(NodeImpl* node);

	inline Engine& engine() const { return _engine; }

private:
	Engine& _engine; ///< Engine we're running in
};


} // namespace Ingen

#endif // MESSAGECONTEXT_H

