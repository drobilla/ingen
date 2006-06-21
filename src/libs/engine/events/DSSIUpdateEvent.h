/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
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

namespace Om {
	
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
	DSSIUpdateEvent(CountedPtr<Responder> responder, const string& path, const string& url);
	
	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	string     m_path;
	string     m_url;
	DSSINode*  m_node;
};


} // namespace Om

#endif // DSSIUPDATEEVENT_H
