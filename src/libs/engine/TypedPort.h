/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef TYPEDPORT_H
#define TYPEDPORT_H

#include <string>
#include "types.h"
#include "Array.h"
#include "Port.h"
#include "Buffer.h"

using std::string;

namespace Om {
	
class MidiMessage;
class Node;


/** A port (with a type).
 *
 * This is basically just a buffer and a bunch of flags and helper methods.
 * All the interesting functionality of ports is in InputPort.
 *
 * \ingroup engine
 */
template <typename T>
class TypedPort : public Port
{
public:
	virtual ~TypedPort();

	void set_value(size_t voice, T val, size_t offset);
	void set_value(T val, size_t offset);
	
	Buffer<T>* buffer(size_t voice) const { return m_buffers.at(voice); }
	
	virtual void process(samplecount nframes);
	virtual void clear_buffers();
	
	/** Used by drivers to prevent port from changing buffers */
	void fixed_buffers(bool b) { m_fixed_buffers = b; }
	bool fixed_buffers()       { return m_fixed_buffers; }

protected:
	TypedPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);
	
	// Prevent copies (undefined)
	TypedPort(const TypedPort<T>& copy);
	TypedPort& operator=(const Port&);

	void allocate_buffers();

	bool m_fixed_buffers;
	
	Array<Buffer<T>*> m_buffers;
};


template class TypedPort<sample>;
template class TypedPort<MidiMessage>;

} // namespace Om

#endif // TYPEDPORT_H
