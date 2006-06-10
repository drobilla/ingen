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

#ifndef LADSPAPLUGIN_H
#define LADSPAPLUGIN_H

#include <string>
#include <ladspa.h>
#include "util/types.h"
#include "NodeBase.h"
#include "Plugin.h"

namespace Om {

class PortInfo;


/** An instance of a LADSPA plugin.
 *
 * \ingroup engine
 */
class LADSPAPlugin : public NodeBase
{
public:
	LADSPAPlugin(const string& name, size_t poly, Patch* parent, const LADSPA_Descriptor* descriptor, samplerate srate, size_t buffer_size);
	virtual ~LADSPAPlugin();

	virtual bool instantiate();

	void activate();
	void deactivate();
	
	void run(size_t nframes);
	
	void set_port_buffer(size_t voice, size_t port_num, void* buf);

	const Plugin* plugin() const  { return m_plugin; }
	void plugin(const Plugin* const pi) { m_plugin = pi; }
	
protected:
	// Prevent copies (undefined)
	LADSPAPlugin(const LADSPAPlugin& copy);
	LADSPAPlugin& operator=(const LADSPAPlugin&);

	void get_port_vals(ulong port_index, PortInfo* info);
	
	const LADSPA_Descriptor* m_descriptor;
	LADSPA_Handle*           m_instances;	

	const Plugin* m_plugin;
};


} // namespace Om

#endif // LADSPAPLUGIN_H
