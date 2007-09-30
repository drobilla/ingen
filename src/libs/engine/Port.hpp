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

#ifndef PORT_H
#define PORT_H

#include <cstdlib>
#include <string>
#include <raul/Array.hpp>
#include "types.hpp"
#include "GraphObject.hpp"
#include "DataType.hpp"

namespace Raul { class Maid; }

namespace Ingen {

class Node;
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
class Port : public GraphObject
{
public:
	virtual ~Port();

	/** A port's parent is always a node, so static cast should be safe */
	Node* parent_node() const { return (Node*)_parent; }
	
	/** Prepare for a new (external) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_poly.
	 */
	virtual bool prepare_poly(uint32_t poly);
	
	/** Apply a new polyphony value.
	 *
	 * Audio thread.
	 *
	 * \param poly Must be < the most recent value passed to prepare_poly.
	 */
	virtual bool apply_poly(Raul::Maid& maid, uint32_t poly);

	Buffer* buffer(uint32_t voice) const { return _buffers->at(voice); }

	/** Called once per process cycle */
	virtual void pre_process(ProcessContext& context) = 0;
	virtual void process(ProcessContext& context) {};
	virtual void post_process(ProcessContext& context) = 0;
	
	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers();
	
	virtual bool is_input()  const = 0;
	virtual bool is_output() const = 0;

	uint32_t num()         const { return _index; }
	uint32_t poly()        const { return _poly; }
	DataType type()        const { return _type; }
	size_t   buffer_size() const { return _buffer_size; }

	virtual void set_buffer_size(size_t size);
	
	void fixed_buffers(bool b) { _fixed_buffers = b; }
	bool fixed_buffers()       { return _fixed_buffers; }
	
	void monitor(bool b) { _monitor = b; }
	bool monitor()       { return _monitor; }

protected:
	Port(Node* const node, const std::string& name, uint32_t index, uint32_t poly, DataType type, size_t buffer_size);
	
	virtual void allocate_buffers();
	virtual void connect_buffers();

	uint32_t _index;
	uint32_t _poly;
	uint32_t _buffer_size;
	DataType _type;
	bool     _fixed_buffers;
	bool     _monitor;

	Raul::Array<Buffer*>* _buffers;

	// Dynamic polyphony
	Raul::Array<Buffer*>* _prepared_buffers;
};


} // namespace Ingen

#endif // PORT_H
