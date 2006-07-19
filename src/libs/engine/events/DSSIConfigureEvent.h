/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef DSSICONFIGUREEVENT_H
#define DSSICONFIGUREEVENT_H

#include "QueuedEvent.h"
#include "DSSINode.h"

namespace Ingen {


/** Change of a 'configure' key/value pair for a DSSI plugin.
 *
 * \ingroup engine
 */
class DSSIConfigureEvent : public QueuedEvent
{
public:
	DSSIConfigureEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& node_path, const string& key, const string& val);

	void pre_process();
	void execute(SampleCount offset);
	void post_process();

private:
	string    m_node_path;
	string    m_key;
	string    m_val;
	DSSINode* m_node;
};


} // namespace Ingen

#endif // DSSICONFIGUREEVENT_H
