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

#ifndef INGEN_ENGINE_DUPLEXPORT_HPP
#define INGEN_ENGINE_DUPLEXPORT_HPP

#include "InputPort.hpp"
#include "PortImpl.hpp"
#include "PortType.hpp"
#include "types.hpp"

#include "ingen/URI.hpp"
#include "ingen/ingen.h"
#include "lv2/urid/urid.h"
#include "raul/Maid.hpp"

#include <boost/intrusive/slist_hook.hpp>

#include <cstddef>
#include <cstdint>

namespace raul { class Symbol; }

namespace ingen {

class Atom;
class Properties;

namespace server {

class BufferFactory;
class Engine;
class GraphImpl;
class RunContext;

/** A duplex Port (both an input and output port on a Graph)
 *
 * This is used for Graph ports, since they need to appear as both an input and
 * an output port based on context.  There are no actual duplex ports in Ingen,
 * a Port is either an Input or Output.  This class only exists to allow Graph
 * outputs to appear as inputs from within that Graph, and vice versa.
 *
 * \ingroup engine
 */
class INGEN_API DuplexPort final
    : public InputPort
    , public boost::intrusive::slist_base_hook<> // In GraphImpl
{
public:
	DuplexPort(BufferFactory&      bufs,
	           GraphImpl*          parent,
	           const raul::Symbol& symbol,
	           uint32_t            index,
	           bool                polyphonic,
	           PortType            type,
	           LV2_URID            buf_type,
	           size_t              buf_size,
	           const Atom&         value,
	           bool                is_output);

	~DuplexPort() override;

	DuplexPort* duplicate(Engine&             engine,
	                      const raul::Symbol& symbol,
	                      GraphImpl*          parent);

	void inherit_neighbour(const PortImpl* port,
	                       Properties&     remove,
	                       Properties&     add) override;

	void on_property(const URI& uri, const Atom& value) override;

	uint32_t max_tail_poly(RunContext& ctx) const override;

	bool prepare_poly(BufferFactory& bufs, uint32_t poly) override;

	bool apply_poly(RunContext& ctx, uint32_t poly) override;

	bool get_buffers(BufferFactory&                   bufs,
	                 PortImpl::GetFn                  get,
	                 const raul::managed_ptr<Voices>& voices,
	                 uint32_t                         poly,
	                 size_t num_in_arcs) const override;

	void set_is_driver_port(BufferFactory& bufs) override;

	/** Set the external driver-provided buffer.
	 *
	 * This may only be called in the process thread, after an earlier call to
	 * prepare_driver_buffer().
	 */
	void set_driver_buffer(void* buf, uint32_t capacity);

	bool
	setup_buffers(RunContext& ctx, BufferFactory& bufs, uint32_t poly) override;

	void pre_process(RunContext& ctx) override;
	void post_process(RunContext& ctx) override;

	SampleCount
	next_value_offset(SampleCount offset, SampleCount end) const override;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_DUPLEXPORT_HPP
