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

#ifndef LV2NODE_H
#define LV2NODE_H

#include <string>
#include <slv2/slv2.h>
#include "types.h"
#include "NodeBase.h"
#include "Plugin.h"

namespace Ingen {


/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Node : public NodeBase
{
public:
	LV2Node(const Plugin*      plugin,
	        const string&      name,
	        size_t             poly,
	        Patch*             parent,
	        SampleRate         srate,
	        size_t             buffer_size);
	
	virtual ~LV2Node();

	virtual bool instantiate();

	void activate();
	void deactivate();
	
	void process(SampleCount nframes, FrameTime start, FrameTime end);
	
	void set_port_buffer(size_t voice, size_t port_num, Buffer* buf);

protected:
	//void get_port_vals(ulong port_index, PortInfo* info);
	
	SLV2Plugin    _lv2_plugin;
	SLV2Instance* _instances;
};


} // namespace Ingen

#endif // LV2NODE_H

