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

#ifndef ADDNODEEVENT_H
#define ADDNODEEVENT_H

#include "QueuedEvent.h"
#include "util/Path.h"
#include <string>
using std::string;

template <typename T> class Array;
template<typename T> class TreeNode;

namespace Om {

class Patch;
class Node;
class Plugin;


/** An event to load a Node and insert it into a Patch.
 *
 * \ingroup engine
 */
class AddNodeEvent : public QueuedEvent
{
public:
	AddNodeEvent(CountedPtr<Responder> responder, const string& path, Plugin* plugin, bool poly);
	~AddNodeEvent();

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	string           m_patch_name;
	Path             m_path;
	Plugin*          m_plugin;
	bool             m_poly;
	Patch*           m_patch;
	Node*            m_node;
	Array<Node*>*    m_process_order; // Patch's new process order
	bool             m_node_already_exists;
};


} // namespace Om

#endif // ADDNODEEVENT_H
