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

namespace Ingen {

class Node;
class Buffer;


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

	Buffer* buffer(size_t voice) const { return _buffers.at(voice); }

	/** Called once per process cycle */
	virtual void pre_process(SampleCount nframes, FrameTime start, FrameTime end) = 0;
	virtual void process(SampleCount nframes, FrameTime start, FrameTime end) {}
	virtual void post_process(SampleCount nframes, FrameTime start, FrameTime end) {};
	
	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers();
	
	virtual bool is_input()  const = 0;
	virtual bool is_output() const = 0;

	size_t   num()         const { return _index; }
	size_t   poly()        const { return _poly; }
	DataType type()        const { return _type; }
	size_t   buffer_size() const { return _buffer_size; }

	virtual void set_buffer_size(size_t size);
	
	void fixed_buffers(bool b) { _fixed_buffers = b; }
	bool fixed_buffers()       { return _fixed_buffers; }

protected:
	Port(Node* const node, const std::string& name, size_t index, size_t poly, DataType type, size_t buffer_size);
	
	virtual void allocate_buffers();
	virtual void connect_buffers();

	size_t    _index;
	size_t    _poly;
	DataType  _type;
	size_t    _buffer_size;
	bool      _fixed_buffers;

	Raul::Array<Buffer*> _buffers;
};


} // namespace Ingen

#endif // PORT_H
