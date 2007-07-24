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

#include "BufferFactory.hpp"
#include "AudioBuffer.hpp"
#include "MidiBuffer.hpp"
#include "OSCBuffer.hpp"

namespace Ingen {
namespace BufferFactory {


Buffer*
create(DataType type, size_t size)
{
	if (type == DataType::FLOAT)
		return new AudioBuffer(size);
	else if (type == DataType::MIDI)
		return new MidiBuffer(size);
	else if (type == DataType::OSC)
		return new OSCBuffer(size);
	else
		return NULL;
}


} // namespace BufferFactory
} // namespace Ingen
