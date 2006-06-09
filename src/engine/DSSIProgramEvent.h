/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DSSIPROGRAMEVENT_H
#define DSSIPROGRAMEVENT_H

#include "QueuedEvent.h"
#include "DSSIPlugin.h"

namespace Om {	


/** A program change for a DSSI plugin.
 *
 * \ingroup engine
 */
class DSSIProgramEvent : public QueuedEvent
{
public:
	DSSIProgramEvent(CountedPtr<Responder> responder, const string& node_path, int bank, int program);

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	string      m_node_path;
	int         m_bank;
	int         m_program;
	DSSIPlugin* m_node;
};


} // namespace Om

#endif // DSSIPROGRAMEVENT_H
