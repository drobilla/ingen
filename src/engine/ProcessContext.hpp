/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_PROCESSCONTEXT_HPP
#define INGEN_ENGINE_PROCESSCONTEXT_HPP

#include "types.hpp"
#include "EventSink.hpp"
#include "Context.hpp"

namespace Ingen {


/** Context of a process() call (the audio context).
 *
 * This class currently adds no functionality to Context, but the
 * separate type is useful for writing functions that must only
 * be run in the audio context (the function taking a ProcessContext
 * parameter makes this clear, and makes breaking the rules obvious).
 *
 * \ingroup engine
 */
class ProcessContext : public Context
{
public:
	ProcessContext(Engine& engine) : Context(engine, AUDIO) {}
};



} // namespace Ingen

#endif // INGEN_ENGINE_PROCESSCONTEXT_HPP

