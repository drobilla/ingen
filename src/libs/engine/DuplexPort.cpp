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

#include "DuplexPort.h"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include "TypedConnection.h"
#include "OutputPort.h"
#include "Node.h"
#include "util.h"

using std::cerr; using std::cout; using std::endl;


namespace Ingen {


template <typename T>
DuplexPort<T>::DuplexPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size, bool is_output)
: TypedPort<T>(parent, name, index, poly, type, buffer_size)
, InputPort<T>(parent, name, index, poly, type, buffer_size)
, OutputPort<T>(parent, name, index, poly, type, buffer_size)
, _is_output(is_output)
{
	assert(TypedPort<T>::_parent == parent);
}
template DuplexPort<Sample>::DuplexPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size, bool is_output);
template DuplexPort<MidiMessage>::DuplexPort(Node* parent, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size, bool is_output);


} // namespace Ingen

