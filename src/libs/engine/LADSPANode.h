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

#ifndef LADSPANODE_H
#define LADSPANODE_H

#include <string>
#include <ladspa.h>
#include "types.h"
#include "NodeBase.h"
#include "Plugin.h"

namespace Om {


/** An instance of a LADSPA plugin.
 *
 * \ingroup engine
 */
class LADSPANode : public NodeBase
{
public:
	LADSPANode(const Plugin* plugin, const string& name, size_t poly, Patch* parent, const LADSPA_Descriptor* descriptor, samplerate srate, size_t buffer_size);
	virtual ~LADSPANode();

	virtual bool instantiate();

	void activate();
	void deactivate();
	
	void process(samplecount nframes);
	
	void set_port_buffer(size_t voice, size_t port_num, void* buf);

	const Plugin* plugin() const  { return _plugin; }
	void plugin(const Plugin* const pi) { _plugin = pi; }
	
protected:
	// Prevent copies (undefined)
	LADSPANode(const LADSPANode& copy);
	LADSPANode& operator=(const LADSPANode&);

	//void get_port_vals(ulong port_index, PortInfo* info);
	
	const LADSPA_Descriptor* _descriptor;
	LADSPA_Handle*           _instances;	
};


} // namespace Om

#endif // LADSPANODE_H
