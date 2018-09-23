/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_LV2BLOCK_HPP
#define INGEN_ENGINE_LV2BLOCK_HPP

#include <mutex>

#include "lilv/lilv.h"
#include "lv2/worker/worker.h"
#include "raul/Maid.hpp"

#include "BufferRef.hpp"
#include "BlockImpl.hpp"
#include "ingen/LV2Features.hpp"
#include "types.hpp"

namespace ingen {
namespace server {

class LV2Plugin;

/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Block : public BlockImpl
{
public:
	LV2Block(LV2Plugin*          plugin,
	         const Raul::Symbol& symbol,
	         bool                polyphonic,
	         GraphImpl*          parent,
	         SampleRate          srate);

	~LV2Block();

	bool instantiate(BufferFactory& bufs, const LilvState* state);

	LilvInstance* instance() { return instance(0); }
	bool          save_state(const FilePath& dir) const;

	BlockImpl* duplicate(Engine&             engine,
	                     const Raul::Symbol& symbol,
	                     GraphImpl*          parent);

	bool prepare_poly(BufferFactory& bufs, uint32_t poly);
	bool apply_poly(RunContext& context, uint32_t poly);

	void activate(BufferFactory& bufs);
	void deactivate();

	LV2_Worker_Status work(uint32_t size, const void* data);

	void run(RunContext& context);
	void post_process(RunContext& context);

	LilvState* load_preset(const URI& uri);

	void apply_state(const UPtr<Worker>& worker, const LilvState* state);

	boost::optional<Resource> save_preset(const URI&        uri,
	                                      const Properties& props);

	void set_port_buffer(uint32_t    voice,
	                     uint32_t    port_num,
	                     BufferRef   buf,
	                     SampleCount offset);

	static LilvState* load_state(World* world, const FilePath& path);

protected:
	struct Instance : public Raul::Noncopyable {
		explicit Instance(LilvInstance* i) : instance(i) {}

		~Instance() { lilv_instance_free(instance); }

		LilvInstance* const instance;
	};

	SPtr<Instance> make_instance(URIs&      uris,
	                             SampleRate rate,
	                             uint32_t   voice,
	                             bool       preparing);

	inline LilvInstance* instance(uint32_t voice) {
		return (LilvInstance*)(*_instances)[voice]->instance;
	}

	typedef Raul::Array< SPtr<Instance> > Instances;

	void drop_instances(const MPtr<Instances>& instances) {
		if (instances) {
			for (size_t i = 0; i < instances->size(); ++i) {
				(*instances)[i].reset();
			}
		}
	}

	struct Response : public Raul::Maid::Disposable
	                , public Raul::Noncopyable
	                , public boost::intrusive::slist_base_hook<>
	{
		inline Response(uint32_t s, const void* d)
			: size(s)
			, data(malloc(s))
		{
			memcpy(data, d, s);
		}

		~Response() {
			free(data);
		}

		const uint32_t size;
		void* const    data;
	};

	typedef boost::intrusive::slist<Response,
	                                boost::intrusive::cache_last<true>,
	                                boost::intrusive::constant_time_size<false>
	                                > Responses;

	static LV2_Worker_Status work_respond(
		LV2_Worker_Respond_Handle handle, uint32_t size, const void* data);

	LV2Plugin*                      _lv2_plugin;
	MPtr<Instances>                 _instances;
	MPtr<Instances>                 _prepared_instances;
	const LV2_Worker_Interface*     _worker_iface;
	std::mutex                      _work_mutex;
	Responses                       _responses;
	SPtr<LV2Features::FeatureArray> _features;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_LV2BLOCK_HPP
