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

#ifndef INGEN_ENGINE_INPUTPORT_HPP
#define INGEN_ENGINE_INPUTPORT_HPP

#include <cassert>
#include <cstdlib>

#include <boost/intrusive/slist.hpp>

#include "ingen/types.hpp"

#include "ArcImpl.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

class ArcImpl;
class Context;
class BlockImpl;
class OutputPort;
class ProcessContext;

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
class InputPort : virtual public PortImpl
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

	virtual ~InputPort() {}

	typedef boost::intrusive::slist<ArcImpl,
	                                boost::intrusive::constant_time_size<false>
	                                > Arcs;

	/** Return the maximum polyphony of an output connected to this input. */
	virtual uint32_t max_tail_poly(Context& context) const;

	bool apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly);

	/** Add an arc.  Realtime safe.
	 *
	 * The buffer of this port will be set directly to the arc's buffer
	 * if there is only one arc, since no copying/mixing needs to take place.
	 *
	 * setup_buffers() must be called later for the change to take effect.
	 */
	void add_arc(ProcessContext& context, ArcImpl* c);

	/** Remove an arc.  Realtime safe.
	 *
	 * setup_buffers() must be called later for the change to take effect.
	 */
	ArcImpl* remove_arc(ProcessContext&   context,
	                    const OutputPort* tail);

	/** Set `voices` as the buffers to be used for this port.
	 *
	 * @return true iff buffers are locally owned by the port
	 */
	bool get_buffers(BufferFactory&      bufs,
	                 Raul::Array<Voice>* voices,
	                 uint32_t            poly,
	                 bool                real_time) const;

	/** Set up buffer pointers. */
	void pre_process(Context& context);

	/** Prepare buffer for access, mixing if necessary. */
	void pre_run(Context& context);

	/** Prepare buffer for next process cycle. */
	void post_process(Context& context);

	SampleCount next_value_offset(SampleCount offset, SampleCount end) const;
	void        update_values(SampleCount offset, uint32_t voice);

	size_t num_arcs() const { return _num_arcs; } ///< Pre-process thread
	void increment_num_arcs() { ++_num_arcs; }
	void decrement_num_arcs() { --_num_arcs; }

	bool is_input()  const { return true; }
	bool is_output() const { return false; }

	bool direct_connect() const;

protected:
	size_t _num_arcs;  ///< Pre-process thread
	Arcs   _arcs;      ///< Audio thread
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_INPUTPORT_HPP
