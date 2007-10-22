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
#include "types.hpp"
#include "NodeBase.hpp"

namespace Ingen {

class LV2Plugin;


/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Node : public NodeBase
{
public:
	LV2Node(LV2Plugin*    plugin,
	        const string& name,
	        bool          polyphonic,
	        PatchImpl*    parent,
	        SampleRate    srate,
	        size_t        buffer_size);
	
	~LV2Node();

	bool instantiate();
	
	bool prepare_poly(uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void activate();
	void deactivate();
	
	void process(ProcessContext& context);
	
	void set_port_buffer(uint32_t voice, uint32_t port_num, Buffer* buf);

protected:
	SLV2Plugin                 _lv2_plugin;
	Raul::Array<SLV2Instance>* _instances;
	Raul::Array<SLV2Instance>* _prepared_instances;
};


} // namespace Ingen

#endif // LV2NODE_H

