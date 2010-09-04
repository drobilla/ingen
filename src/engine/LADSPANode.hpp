/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_LADSPANODE_HPP
#define INGEN_ENGINE_LADSPANODE_HPP

#include <string>
#include <ladspa.h>
#include <boost/optional.hpp>
#include "raul/IntrusivePtr.hpp"
#include "types.hpp"
#include "NodeImpl.hpp"
#include "PluginImpl.hpp"

namespace Ingen {


/** An instance of a LADSPA plugin.
 *
 * \ingroup engine
 */
class LADSPANode : public NodeImpl
{
public:
	LADSPANode(PluginImpl*              plugin,
	           const std::string&       name,
	           bool                     polyphonic,
	           PatchImpl*               parent,
	           const LADSPA_Descriptor* descriptor,
	           SampleRate               srate);

	~LADSPANode();

	bool instantiate(BufferFactory& bufs);

	bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void activate(BufferFactory& bufs);
	void deactivate();

	void process(ProcessContext& context);

	void set_port_buffer(uint32_t voice, uint32_t port_num,
			IntrusivePtr<Buffer> buf, SampleCount offset);

protected:
	inline LADSPA_Handle instance(uint32_t voice) { return (*_instances)[voice]->handle; }

	void get_port_limits(unsigned long            port_index,
	                     boost::optional<Sample>& default_value,
	                     boost::optional<Sample>& lower_bound,
	                     boost::optional<Sample>& upper_bound);

	struct Instance {
		Instance(const LADSPA_Descriptor* d=NULL, LADSPA_Handle h=NULL)
			: descriptor(d), handle(h)
		{}
		~Instance() {
			if (descriptor && handle)
				descriptor->cleanup(handle);
		}
		const LADSPA_Descriptor* descriptor;
		LADSPA_Handle            handle;
	};

	typedef Raul::Array< SharedPtr<Instance> > Instances;

	const LADSPA_Descriptor* _descriptor;
	Instances*               _instances;
	Instances*               _prepared_instances;
};


} // namespace Ingen

#endif // INGEN_ENGINE_LADSPANODE_HPP
