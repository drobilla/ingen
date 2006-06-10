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

#ifndef LV2PLUGIN_H
#define LV2PLUGIN_H

#include <string>
#include <slv2/slv2.h>
#include "util/types.h"
#include "NodeBase.h"
#include "Plugin.h"

namespace Om {

class PortInfo;


/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Plugin : public NodeBase
{
public:
	LV2Plugin(const string&      name,
	          size_t             poly,
	          Patch*             parent,
	          const SLV2Plugin*  plugin,
	          samplerate         srate,
	          size_t             buffer_size);
	
	virtual ~LV2Plugin();

	virtual bool instantiate();

	void activate();
	void deactivate();
	
	void run(size_t nframes);
	
	void set_port_buffer(size_t voice, size_t port_num, void* buf);

	const Plugin* plugin() const       { return m_om_plugin; }
	void plugin(const Plugin* const p) { m_om_plugin = p; }
	
protected:
	// Prevent copies (undefined)
	LV2Plugin(const LV2Plugin& copy);
	LV2Plugin& operator=(const LV2Plugin&);

	void get_port_vals(ulong port_index, PortInfo* info);
	
	const SLV2Plugin* m_lv2_plugin;
	SLV2Instance**    m_instances;

	const Plugin* m_om_plugin;
};


} // namespace Om

#endif // LV2PLUGIN_H

