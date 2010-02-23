/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_LV2NODE_HPP
#define INGEN_ENGINE_LV2NODE_HPP

#include <string>
#include "slv2/slv2.h"
#include "raul/IntrusivePtr.hpp"
#include "contexts.lv2/contexts.h"
#include "types.hpp"
#include "NodeBase.hpp"
#include "LV2Features.hpp"

namespace Ingen {

class LV2Plugin;


/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Node : public NodeBase
{
public:
	LV2Node(LV2Plugin*         plugin,
	        const std::string& name,
	        bool               polyphonic,
	        PatchImpl*         parent,
	        SampleRate         srate);

	~LV2Node();

	bool instantiate(BufferFactory& bufs);

	bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void activate(BufferFactory& bufs);
	void deactivate();

	void message_run(MessageContext& context);

	void process(ProcessContext& context);

	void set_port_buffer(uint32_t voice, uint32_t port_num, IntrusivePtr<Buffer> buf);

protected:
	inline SLV2Instance instance(uint32_t voice) { return (SLV2Instance)(*_instances)[voice].get(); }

	typedef Raul::Array< SharedPtr<void> > Instances;

	LV2Plugin* _lv2_plugin;
	Instances* _instances;
	Instances* _prepared_instances;

	LV2MessageContext* _message_funcs;

	SharedPtr<Shared::LV2Features::FeatureArray> _features;
};


} // namespace Ingen

#endif // INGEN_ENGINE_LV2NODE_HPP

