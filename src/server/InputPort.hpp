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

#ifndef INGEN_ENGINE_INPUTPORT_HPP
#define INGEN_ENGINE_INPUTPORT_HPP

#include "ArcImpl.hpp" // IWYU pragma: keep
#include "PortImpl.hpp"
#include "PortType.hpp"
#include "types.hpp"

#include "lv2/urid/urid.h"
#include "raul/Maid.hpp"

#include <boost/intrusive/slist.hpp>

#include <cstdint>
#include <cstdlib>

namespace Raul { class Symbol; }

namespace boost {
namespace intrusive {

template <bool Enabled> struct constant_time_size;

} // namespace intrusive
} // namespace boost

namespace ingen {

class Atom;

namespace server {

class BlockImpl;
class BufferFactory;
class RunContext;

/** An input port on a Block or Graph.
 *
 * All ports have a Buffer, but the actual contents (data) of that buffer may be
 * set directly to the incoming arc's buffer if there's only one inbound
 * arc, to eliminate the need to copy/mix.
 *
 * If a port has multiple arcs, they will be mixed down into the local
 * buffer and it will be used.
 *
 * \ingroup engine
 */
class InputPort : public PortImpl
{
public:
	InputPort(BufferFactory&      bufs,
	          BlockImpl*          parent,
	          const Raul::Symbol& symbol,
	          uint32_t            index,
	          uint32_t            poly,
	          PortType            type,
	          LV2_URID            buffer_type,
	          const Atom&         value,
	          size_t              buffer_size = 0);

	using Arcs =
	        boost::intrusive::slist<ArcImpl,
	                                boost::intrusive::constant_time_size<true>>;

	/** Return the maximum polyphony of an output connected to this input. */
	virtual uint32_t max_tail_poly(RunContext& ctx) const;

	bool apply_poly(RunContext& ctx, uint32_t poly) override;

	/** Add an arc.  Realtime safe.
	 *
	 * The buffer of this port will be set directly to the arc's buffer
	 * if there is only one arc, since no copying/mixing needs to take place.
	 *
	 * setup_buffers() must be called later for the change to take effect.
	 */
	void add_arc(RunContext& ctx, ArcImpl& c);

	/** Remove an arc.  Realtime safe.
	 *
	 * setup_buffers() must be called later for the change to take effect.
	 */
	void remove_arc(ArcImpl& arc);

	/** Like `get_buffers`, but for the pre-process thread.
	 *
	 * This uses the "current" number of arcs fromthe perspective of the
	 * pre-process thread to allocate buffers for application of a
	 * connection/disconnection/etc in the next process cycle.
	 */
	bool pre_get_buffers(BufferFactory&             bufs,
	                     Raul::managed_ptr<Voices>& voices,
	                     uint32_t                   poly) const;

	bool
	setup_buffers(RunContext& ctx, BufferFactory& bufs, uint32_t poly) override;

	/** Set up buffer pointers. */
	void pre_process(RunContext& ctx) override;

	/** Prepare buffer for access, mixing if necessary. */
	void pre_run(RunContext& ctx) override;

	/** Prepare buffer for next process cycle. */
	void post_process(RunContext& ctx) override;

	SampleCount
	next_value_offset(SampleCount offset, SampleCount end) const override;

	size_t num_arcs() const override { return _num_arcs; }
	void   increment_num_arcs() { ++_num_arcs; }
	void   decrement_num_arcs() { --_num_arcs; }

	bool direct_connect() const;

protected:
	bool get_buffers(BufferFactory&                   bufs,
	                 PortImpl::GetFn                  get,
	                 const Raul::managed_ptr<Voices>& voices,
	                 uint32_t                         poly,
	                 size_t num_in_arcs) const override;

	size_t _num_arcs;  ///< Pre-process thread
	Arcs   _arcs;      ///< Audio thread
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_INPUTPORT_HPP
