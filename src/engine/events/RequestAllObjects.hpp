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

#ifndef REQUESTALLOBJECTSEVENT_H
#define REQUESTALLOBJECTSEVENT_H

#include "QueuedEvent.hpp"

namespace Ingen {
namespace Events {


/** A request from a client to send notification of all objects (ie refresh).
 *
 * \ingroup engine
 */
class RequestAllObjects : public QueuedEvent
{
public:
	RequestAllObjects(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp);

	void pre_process();
	void post_process();
};


} // namespace Ingen
} // namespace Events

#endif // REQUESTALLOBJECTSEVENT_H