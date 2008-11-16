/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef PORTIMPL_H
#define PORTIMPL_H

#include <cstdlib>
#include <string>
#include "raul/Array.hpp"
#include "interface/Port.hpp"
#include "types.hpp"
#include "GraphObjectImpl.hpp"
#include "interface/DataType.hpp"
#include "Buffer.hpp"
#include "Context.hpp"

namespace Raul { class Maid; class Atom; }

namespace Ingen {

class NodeImpl;
class Buffer;
class ProcessContext;


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
	virtual ~PortImpl();

	/** A port's parent is always a node, so static cast should be safe */
	NodeImpl* parent_node() const { return (NodeImpl*)_parent; }
	
	bool set_polyphonic(Raul::Maid& maid, bool p);
	
	/** Prepare for a new (external) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_poly.
	 */
	virtual bool prepare_poly(uint32_t poly);
	
	/** Apply a new polyphony value.
	 *
	 * Audio thread.
	 *
	 * \a poly Must be < the most recent value passed to prepare_poly.
	 */
	virtual bool apply_poly(Raul::Maid& maid, uint32_t poly);
	
	const Raul::Atom& value() const { return _value; }
	void              set_value(const Raul::Atom& v) { _value = v; }

	inline Buffer* buffer(uint32_t voice) const {
		Buffer* const buf = _buffers->at(voice);
		if (buf->is_joined()) {
			assert(buf->joined_buffer());
			return buf->joined_buffer();
		} else {
			return buf;
		}
	}
	inline Buffer* prepared_buffer(uint32_t voice) const {
		return _prepared_buffers->at(voice);
	}

	/** Called once per process cycle */
	virtual void pre_process(ProcessContext& context) = 0;
	virtual void process(ProcessContext& context) {};
	virtual void post_process(ProcessContext& context) = 0;
	
	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers();

	virtual bool is_input()  const = 0;
	virtual bool is_output() const = 0;

	uint32_t     index()       const { return _index; }
	uint32_t     poly()        const { return _poly; }
	DataType     type()        const { return _type; }
	size_t       buffer_size() const { return (_type == DataType::CONTROL) ? 1 : _buffer_size; }

	virtual void set_buffer_size(size_t size);
	
	void fixed_buffers(bool b) { _fixed_buffers = b; }
	bool fixed_buffers()       { return _fixed_buffers; }
	
	void broadcast(bool b) { _broadcast = b; }
	bool broadcast()       { return _broadcast; }

	void raise_set_by_user_flag() { _set_by_user = true; }

	Context::ID context() const            { return _context; }
	void        set_context(Context::ID c) { _context = c; }

protected:
	PortImpl(NodeImpl*          node,
	         const std::string& name,
	         uint32_t           index,
	         uint32_t           poly,
	         DataType           type,
	         const Raul::Atom&  value,
	         size_t             buffer_size);
	
	virtual void allocate_buffers();
	virtual void connect_buffers();
	virtual void broadcast(ProcessContext& context);

	uint32_t   _index;
	uint32_t   _poly;
	uint32_t   _buffer_size;
	DataType   _type;
	Raul::Atom _value;
	bool       _fixed_buffers;
	bool       _broadcast;
	bool       _set_by_user;
	Sample     _last_broadcasted_value;

	Context::ID           _context;
	Raul::Array<Buffer*>* _buffers;

	// Dynamic polyphony
	Raul::Array<Buffer*>* _prepared_buffers;
};


} // namespace Ingen

#endif // PORTIMPL_H
