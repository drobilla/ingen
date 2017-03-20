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

#include <boost/intrusive/slist.hpp>

#include "BufferRef.hpp"
#include "InputPort.hpp"

namespace Ingen {
namespace Server {

class BlockImpl;

/** A duplex Port (both an input and output port on a Graph)
 *
 * This is used for Graph ports, since they need to appear as both an input and
 * an output port based on context.  There are no actual duplex ports in Ingen,
 * a Port is either an Input or Output.  This class only exists to allow Graph
 * outputs to appear as inputs from within that Graph, and vice versa.
 *
 * \ingroup engine
 */
class DuplexPort : public InputPort
                 , public boost::intrusive::slist_base_hook<>  // In GraphImpl
{
public:
	DuplexPort(BufferFactory&      bufs,
	           GraphImpl*          parent,
	           const Raul::Symbol& symbol,
	           uint32_t            index,
	           bool                polyphonic,
	           PortType            type,
	           LV2_URID            buf_type,
	           size_t              buf_size,
	           const Atom&         value,
	           bool                is_output);

	virtual ~DuplexPort();

	DuplexPort* duplicate(Engine&             engine,
	                      const Raul::Symbol& symbol,
	                      GraphImpl*          parent);

	void inherit_neighbour(const PortImpl* port,
	                       Properties&     remove,
	                       Properties&     add);

	void on_property(const Raul::URI& uri, const Atom& value);

	uint32_t max_tail_poly(RunContext& context) const;

	bool prepare_poly(BufferFactory& bufs, uint32_t poly);

	bool apply_poly(RunContext& context, uint32_t poly);

	bool get_buffers(BufferFactory&      bufs,
	                 PortImpl::GetFn     get,
	                 const MPtr<Voices>& voices,
	                 uint32_t            poly,
	                 size_t              num_in_arcs) const;

	virtual void set_is_driver_port(BufferFactory& bufs);

	/** Set the external driver-provided buffer.
	 *
	 * This may only be called in the process thread, after an earlier call to
	 * prepare_driver_buffer().
	 */
	void set_driver_buffer(void* buf, uint32_t capacity);

	bool setup_buffers(RunContext& ctx, BufferFactory& bufs, uint32_t poly);

	void pre_process(RunContext& context);
	void post_process(RunContext& context);

	SampleCount next_value_offset(SampleCount offset, SampleCount end) const;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_DUPLEXPORT_HPP
