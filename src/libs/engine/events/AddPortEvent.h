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

#ifndef ADDPORTEVENT_H
#define ADDPORTEVENT_H

#include "QueuedEvent.h"
#include "util/Path.h"
#include "DataType.h"
#include "Array.h"
#include <string>
using std::string;

template <typename T> class Array;

namespace Om {

class Patch;
class Port;
class Plugin;


/** An event to add a Port to a Patch.
 *
 * \ingroup engine
 */
class AddPortEvent : public QueuedEvent
{
public:
	AddPortEvent(CountedPtr<Responder> responder, const string& path, const string& type, bool is_output);
	~AddPortEvent();

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	Path          _path;
	string        _type;
	bool          _is_output;
	DataType      _data_type;
	Patch*        _patch;
	Port*         _port;
	Array<Port*>* _ports_array; // New (external) ports array for Patch
	bool          _succeeded;
	/*
	string           m_patch_name;
	Path             m_path;
	Plugin*          m_plugin;
	bool             m_poly;
	Patch*           m_patch;
	Node*            m_node;
	Array<Node*>*    m_process_order; // Patch's new process order
	bool             m_node_already_exists;*/
};


} // namespace Om

#endif // ADDPORTEVENT_H
