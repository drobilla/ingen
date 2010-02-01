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

#ifndef INGEN_EVENTS_LEARN_HPP
#define INGEN_EVENTS_LEARN_HPP

#include "QueuedEvent.hpp"
#include "internals/Controller.hpp"
#include "types.hpp"

namespace Ingen {

class NodeImpl;

namespace Events {


/** A MIDI learn event (used by control node to learn controller number).
 *
 * \ingroup engine
 */
class Learn : public QueuedEvent
{
public:
	Learn(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const Raul::Path& path);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType {
		NO_ERROR,
		INVALID_NODE_TYPE
	};

	ErrorType        _error;
	const Raul::Path _path;
	GraphObjectImpl* _object;
	bool             _done;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_LEARN_HPP
