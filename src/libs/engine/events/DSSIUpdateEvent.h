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

#ifndef DSSIUPDATEEVENT_H
#define DSSIUPDATEEVENT_H

#include "QueuedEvent.h"
#include <string>

using std::string;

namespace Ingen {
	
class DSSINode;


/** A DSSI "update" responder for a DSSI plugin/node.
 *
 * This sends all information about the plugin to the UI (over OSC).
 *
 * \ingroup engine
 */
class DSSIUpdateEvent : public QueuedEvent
{
public:
	DSSIUpdateEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, const string& url);
	
	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	string     _path;
	string     _url;
	DSSINode*  _node;
};


} // namespace Ingen

#endif // DSSIUPDATEEVENT_H
