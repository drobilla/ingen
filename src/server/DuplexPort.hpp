/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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
#include "OutputPort.hpp"

namespace Ingen {
namespace Server {

class BlockImpl;

/** A duplex Port (both an InputPort and an OutputPort on a Graph)
 *
 * This is used for Graph ports, since they need to appear as both an input and
 * an output port based on context.  There are no actual duplex ports in Ingen,
 * a Port is either an Input or Output.  This class only exists to allow Graph
 * outputs to appear as inputs from within that Graph, and vice versa.
 *
 * \ingroup engine
 */
class DuplexPort : public InputPort
                 , public OutputPort
                 , public boost::intrusive::slist_base_hook<>  // In GraphImpl
{
public:
	DuplexPort(BufferFactory&      bufs,
	           BlockImpl*          parent,
	           const Raul::Symbol& symbol,
	           uint32_t            index,
	           bool                polyphonic,
	           uint32_t            poly,
	           PortType            type,
	           LV2_URID            buffer_type,
	           const Atom&         value,
	           size_t              buffer_size,
	           bool                is_output);

	virtual ~DuplexPort();

	DuplexPort* duplicate(Engine&             engine,
	                      const Raul::Symbol& symbol,
	                      GraphImpl*          parent);

	void inherit_neighbour(const PortImpl*       port,
	                       Resource::Properties& remove,
	                       Resource::Properties& add);

	void on_property(const Raul::URI& uri, const Atom& value);

	uint32_t max_tail_poly(Context& context) const;

	bool prepare_poly(BufferFactory& bufs, uint32_t poly);

	bool apply_poly(ProcessContext& context, Raul::Maid& maid, uint32_t poly);

	bool get_buffers(BufferFactory&      bufs,
	                 Raul::Array<Voice>* voices,
	                 uint32_t            poly,
	                 bool                real_time) const;

	void pre_process(Context& context);
	void post_process(Context& context);

	SampleCount next_value_offset(SampleCount offset, SampleCount end) const;
	void        update_values(SampleCount offset, uint32_t voice);

	bool is_input()  const { return !_is_output; }
	bool is_output() const { return _is_output; }

protected:
	bool _is_output;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_DUPLEXPORT_HPP
