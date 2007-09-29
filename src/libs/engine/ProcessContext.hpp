/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef PROCESSCONTEXT_H
#define PROCESSCONTEXT_H

#include "EventSink.hpp"

namespace Ingen {


/** Context of a process() call.
 *
 * This is used to pass whatever information a GraphObject might need to
 * process in the audio thread, e.g. the available thread pool, sink for
 * events (generated in the audio thread, not user initiated events), etc.
 *
 * \ingroup engine
 */
class ProcessContext
{
public:
	ProcessContext() : _event_sink(1024) {} // FIXME: size?
	
	EventSink& event_sink() { return _event_sink; }

private:
	EventSink _event_sink;
};



} // namespace Ingen

#endif // PROCESSCONTEXT_H

