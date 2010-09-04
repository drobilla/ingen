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

#ifndef INGEN_ENGINE_PORTIMPL_HPP
#define INGEN_ENGINE_PORTIMPL_HPP

#include <cstdlib>
#include <string>
#include <set>
#include "raul/Array.hpp"
#include "raul/Atom.hpp"
#include "interface/Port.hpp"
#include "types.hpp"
#include "GraphObjectImpl.hpp"
#include "interface/PortType.hpp"
#include "Buffer.hpp"
#include "Context.hpp"

namespace Raul { class Maid; }

namespace Ingen {

class NodeImpl;
class Buffer;
class BufferFactory;


/** A port on a Node.
 *
 * This is a non-template abstract base class, which basically exists so
 * things can pass around Port pointers and not have to worry about type,
 * templates, etc.
 *
 * \ingroup engine
 */
class PortImpl : public GraphObjectImpl, public Ingen::Shared::Port
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
	Raul::Array<BufferFactory::Ref>* set_buffers(Raul::Array<BufferFactory::Ref>* buffers);

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
	virtual bool apply_poly(Raul::Maid& maid, uint32_t poly);

	const Raul::Atom& value() const { return _value; }
	void              set_value(const Raul::Atom& v) { _value = v; }

	inline BufferFactory::Ref buffer(uint32_t voice) const {
		return _buffers->at((_poly == 1) ? 0 : voice);
	}
	inline BufferFactory::Ref prepared_buffer(uint32_t voice) const {
		return _prepared_buffers->at(voice);
	}

	/** Called once per process cycle */
	virtual void pre_process(Context& context) = 0;
	virtual void post_process(Context& context) = 0;

	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers();

	virtual bool get_buffers(BufferFactory& bufs, Raul::Array<BufferFactory::Ref>* buffers, uint32_t poly) = 0;

	void setup_buffers(BufferFactory& bufs, uint32_t poly) {
		get_buffers(bufs, _buffers, poly);
	}

	virtual void connect_buffers(SampleCount offset=0);
	virtual void recycle_buffers();

	virtual bool is_input()  const = 0;
	virtual bool is_output() const = 0;

	uint32_t index() const { return _index; }

	const PortTypes& types() const { return _types; }

	PortType buffer_type() const;

	bool supports(const Raul::URI& value_type) const;

	size_t buffer_size() const { return _buffer_size; }

	uint32_t poly() const {
		return _poly;
	}
	uint32_t prepared_poly() const {
		return (_prepared_buffers) ? _prepared_buffers->size() : 1;
	}

	void set_buffer_size(Context& context, BufferFactory& bufs, size_t size);
	void set_buffer_type(PortType type);

	void broadcast(bool b) { _broadcast = b; }
	bool broadcast()       { return _broadcast; }

	void broadcast_value(Context& context, bool force=false);

	void raise_set_by_user_flag() { _set_by_user = true; }

	Context::ID context() const            { return _context; }
	void        set_context(Context::ID c);

	BufferFactory& bufs() const { return _bufs; }

protected:
	PortImpl(BufferFactory&      bufs,
	         NodeImpl*           node,
	         const Raul::Symbol& name,
	         uint32_t            index,
	         uint32_t            poly,
	         Shared::PortType    type,
	         const Raul::Atom&   value,
	         size_t              buffer_size);

	BufferFactory&             _bufs;
	uint32_t                   _index;
	uint32_t                   _poly;
	uint32_t                   _buffer_size;
	PortType                   _buffer_type;
	std::set<Shared::PortType> _types;
	Raul::Atom                 _value;
	bool                       _broadcast;
	bool                       _set_by_user;
	Raul::Atom                 _last_broadcasted_value;

	Context::ID                      _context;
	Raul::Array<BufferFactory::Ref>* _buffers;

	// Dynamic polyphony
	Raul::Array<BufferFactory::Ref>* _prepared_buffers;
};


} // namespace Ingen

#endif // INGEN_ENGINE_PORTIMPL_HPP
