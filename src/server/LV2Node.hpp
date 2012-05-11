/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_LV2NODE_HPP
#define INGEN_ENGINE_LV2NODE_HPP

#include <string>

#include "lilv/lilv.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"

#include "BufferRef.hpp"
#include "NodeImpl.hpp"
#include "ingen/shared/LV2Features.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class LV2Plugin;

/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Node : public NodeImpl
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

	void work(MessageContext& context, uint32_t size, const void* data);

	void process(ProcessContext& context);

	void set_port_buffer(uint32_t    voice,
	                     uint32_t    port_num,
	                     BufferRef   buf,
	                     SampleCount offset);

protected:
	inline LilvInstance* instance(uint32_t voice) {
		return (LilvInstance*)(*_instances)[voice].get();
	}

	typedef Raul::Array< SharedPtr<void> > Instances;

	LV2Plugin*                                   _lv2_plugin;
	Instances*                                   _instances;
	Instances*                                   _prepared_instances;
	LV2_Worker_Interface*                        _worker_iface;
	SharedPtr<Shared::LV2Features::FeatureArray> _features;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2NODE_HPP

