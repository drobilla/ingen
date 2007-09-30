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

#ifndef DSSICONTROLEVENT_H
#define DSSICONTROLEVENT_H

#include "QueuedEvent.hpp"
#include "DSSINode.hpp"

namespace Ingen {
	

/** A control change event for a DSSI plugin.
 *
 * This does essentially the same thing as a SetPortValueEvent.
 *
 * \ingroup engine
 */
class DSSIControlEvent : public QueuedEvent
{
public:
	DSSIControlEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, int port_num, Sample val);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	string    _node_path;
	int       _port_num;
	float     _val;
	DSSINode* _node;
};


} // namespace Ingen

#endif // DSSICONTROLEVENT_H
