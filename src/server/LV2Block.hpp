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

#include "BlockImpl.hpp"
#include "BufferRef.hpp"
#include "State.hpp"
#include "types.hpp"

#include <ingen/LV2Features.hpp>
#include <lilv/lilv.h>
#include <lv2/worker/worker.h>
#include <raul/Array.hpp>
#include <raul/Maid.hpp>
#include <raul/Noncopyable.hpp>

#include <boost/intrusive/options.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/slist_hook.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>

namespace raul {
class Symbol;
} // namespace raul

namespace ingen {

class URIs;
class World;

namespace server {

class BufferFactory;
class GraphImpl;
class LV2Plugin;

/** An instance of a LV2 plugin.
 *
 * \ingroup engine
 */
class LV2Block final : public BlockImpl
{
public:
	LV2Block(LV2Plugin*          plugin,
	         const raul::Symbol& symbol,
	         bool                polyphonic,
	         GraphImpl*          parent,
	         SampleRate          srate);

	~LV2Block() override;

	bool instantiate(BufferFactory& bufs, const LilvState* state);

	LilvInstance* instance() override { return instance(0); }
	bool          save_state(const std::filesystem::path& dir) const override;

	BlockImpl* duplicate(Engine&             engine,
	                     const raul::Symbol& symbol,
	                     GraphImpl*          parent) override;

	bool prepare_poly(BufferFactory& bufs, uint32_t poly) override;
	bool apply_poly(RunContext& ctx, uint32_t poly) override;

	void activate(BufferFactory& bufs) override;
	void deactivate() override;

	LV2_Worker_Status work(uint32_t size, const void* data);

	void run(RunContext& ctx) override;
	void post_process(RunContext& ctx) override;

	StatePtr load_preset(const URI& uri) override;

	void apply_state(const std::unique_ptr<Worker>& worker,
	                 const LilvState*               state) override;

	std::optional<Resource> save_preset(const URI&        uri,
	                                    const Properties& props) override;

	void set_port_buffer(uint32_t         voice,
	                     uint32_t         port_num,
	                     const BufferRef& buf,
	                     SampleCount      offset) override;

	static StatePtr load_state(World& world, const std::filesystem::path& path);

protected:
	struct Instance : public raul::Noncopyable {
		explicit Instance(LilvInstance* i) noexcept : instance(i) {}

		~Instance() { lilv_instance_free(instance); }

		LilvInstance* const instance;
	};

	std::shared_ptr<Instance>
	make_instance(URIs& uris, SampleRate rate, uint32_t voice, bool preparing);

	LilvInstance* instance(uint32_t voice) {
		return static_cast<LilvInstance*>((*_instances)[voice]->instance);
	}

	using Instances = raul::Array<std::shared_ptr<Instance>>;

	static void drop_instances(const raul::managed_ptr<Instances>& instances) {
		if (instances) {
			for (size_t i = 0; i < instances->size(); ++i) {
				(*instances)[i].reset();
			}
		}
	}

	struct Response : public raul::Maid::Disposable
	                , public raul::Noncopyable
	                , public boost::intrusive::slist_base_hook<> {
		Response(uint32_t s, const void* d)
			: size(s)
			, data(malloc(s))
		{
			memcpy(data, d, s);
		}

		~Response() override {
			free(data);
		}

		const uint32_t size;
		void* const    data;
	};

	using Responses = boost::intrusive::slist<
	        Response,
	        boost::intrusive::cache_last<true>,
	        boost::intrusive::constant_time_size<false>>;

	static LV2_Worker_Status work_respond(
		LV2_Worker_Respond_Handle handle, uint32_t size, const void* data);

	LV2Plugin*                                 _lv2_plugin;
	raul::managed_ptr<Instances>               _instances;
	raul::managed_ptr<Instances>               _prepared_instances;
	const LV2_Worker_Interface*                _worker_iface{nullptr};
	std::mutex                                 _work_mutex;
	Responses                                  _responses;
	std::shared_ptr<LV2Features::FeatureArray> _features;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_LV2BLOCK_HPP
