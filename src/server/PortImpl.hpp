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

#ifndef INGEN_ENGINE_PORTIMPL_HPP
#define INGEN_ENGINE_PORTIMPL_HPP

#include <cstdlib>
#include <string>

#include "raul/Array.hpp"
#include "raul/Atom.hpp"

#include "ingen/Port.hpp"

#include "Buffer.hpp"
#include "BufferRef.hpp"
#include "Context.hpp"
#include "GraphObjectImpl.hpp"
#include "PortType.hpp"
#include "types.hpp"

namespace Raul { class Maid; }

namespace Ingen {
namespace Server {

class NodeImpl;
class BufferFactory;

/** A port on a Node.
 *
 * This is a non-template abstract base class, which basically exists so
 * things can pass around Port pointers and not have to worry about type,
 * templates, etc.
 *
 * \ingroup engine
 */
class PortImpl : public GraphObjectImpl, public Port
{
public:
	~PortImpl();

	/** A port's parent is always a node, so static cast should be safe */
	NodeImpl* parent_node() const { return (NodeImpl*)_parent; }

	/** Set the buffers array for this port.
	 *
	 * Audio thread.  Returned value must be freed by caller.
	 * \a buffers must be poly() long
	 */
	Raul::Array<BufferRef>* set_buffers(ProcessContext&         context,
	                                    Raul::Array<BufferRef>* buffers);

	/** Prepare for a new (external) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_poly.
	 */
	virtual bool prepare_poly(BufferFactory& bufs, uint32_t poly);

	virtual void prepare_poly_buffers(BufferFactory& bufs);

	/** Apply a new polyphony value.
	 *
	 * Audio thread.
	 * \a poly Must be < the most recent value passed to prepare_poly.
	 */
	virtual bool apply_poly(
		ProcessContext& context, Raul::Maid& maid, uint32_t poly);

	const Raul::Atom& value() const { return _value; }
	void              set_value(const Raul::Atom& v) { _value = v; }

	const Raul::Atom& minimum() const { return _min; }
	const Raul::Atom& maximum() const { return _max; }

	/* The following two methods store the range in variables so it can be
	   accessed in the process thread, which is required for applying control
	   bindings from incoming MIDI data.
	*/
	void set_minimum(const Raul::Atom& min) { _min = min; }
	void set_maximum(const Raul::Atom& max) { _max = max; }

	inline BufferRef buffer(uint32_t voice) const {
		return _buffers->at((_poly == 1) ? 0 : voice);
	}
	inline BufferRef prepared_buffer(uint32_t voice) const {
		return _prepared_buffers->at(voice);
	}

	/** Called once per process cycle */
	virtual void pre_process(Context& context) = 0;
	virtual void post_process(Context& context) = 0;

	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers();

	virtual bool get_buffers(BufferFactory&          bufs,
	                         Raul::Array<BufferRef>* buffers,
	                         uint32_t                poly) const = 0;

	void setup_buffers(BufferFactory& bufs, uint32_t poly) {
		get_buffers(bufs, _buffers, poly);
	}

	virtual void connect_buffers(SampleCount offset=0);
	virtual void recycle_buffers();

	virtual bool is_input()  const = 0;
	virtual bool is_output() const = 0;

	uint32_t index() const { return _index; }

	inline bool is_a(PortType type) const { return _type == type; }

	PortType type()        const { return _type; }
	LV2_URID buffer_type() const { return _buffer_type; }

	bool supports(const Raul::URI& value_type) const;

	size_t buffer_size() const { return _buffer_size; }

	uint32_t poly() const {
		return _poly;
	}
	uint32_t prepared_poly() const {
		return (_prepared_buffers) ? _prepared_buffers->size() : 1;
	}

	void set_buffer_size(Context& context, BufferFactory& bufs, size_t size);

	void broadcast(bool b) { _broadcast = b; }
	bool broadcast()       { return _broadcast; }

	void broadcast_value(Context& context, bool force=false);

	void raise_set_by_user_flag() { _set_by_user = true; }

	BufferFactory& bufs() const { return _bufs; }

protected:
	PortImpl(BufferFactory&      bufs,
	         NodeImpl*           node,
	         const Raul::Symbol& name,
	         uint32_t            index,
	         uint32_t            poly,
	         PortType            type,
	         LV2_URID            buffer_type,
	         const Raul::Atom&   value,
	         size_t              buffer_size);

	BufferFactory&          _bufs;
	uint32_t                _index;
	uint32_t                _poly;
	uint32_t                _buffer_size;
	PortType                _type;
	LV2_URID                _buffer_type;
	Raul::Atom              _value;
	Raul::Atom              _min;
	Raul::Atom              _max;
	Raul::Atom              _last_broadcasted_value;
	Raul::Array<BufferRef>* _buffers;
	Raul::Array<BufferRef>* _prepared_buffers;
	bool                    _broadcast;
	bool                    _set_by_user;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PORTIMPL_HPP
