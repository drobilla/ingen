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

#ifndef SETPOLYPHONYEVENT_H
#define SETPOLYPHONYEVENT_H

#include <string>
#include "raul/Array.hpp"
#include "QueuedEvent.hpp"

using std::string;

namespace Ingen {

class PatchImpl;


/** Delete all nodes from a patch.
 *
 * \ingroup engine
 */
class SetPolyphonyEvent : public QueuedEvent
{
public:
	SetPolyphonyEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& patch_path, uint32_t poly);
	
	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	const string   _patch_path;
	PatchImpl*     _patch;
	const uint32_t _poly;

};


} // namespace Ingen


#endif // SETPOLYPHONYEVENT_H
