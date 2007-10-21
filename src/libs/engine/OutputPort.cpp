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

#include <iostream>
#include "OutputPort.hpp"
#include "Buffer.hpp"
#include "ProcessContext.hpp"

using namespace std;

namespace Ingen {

	
OutputPort::OutputPort(NodeImpl*     parent,
                       const string& name,
                       uint32_t      index,
                       uint32_t      poly,
                       DataType      type,
                       size_t        buffer_size)
	: PortImpl(parent, name, index, poly, type, buffer_size)
{
	if (type == DataType::CONTROL)
		_broadcast = true;
}


void
OutputPort::pre_process(ProcessContext& context)
{
	for (uint32_t i=0; i < _poly; ++i)
		buffer(i)->prepare_write(context.nframes());
}


void
OutputPort::post_process(ProcessContext& context)
{
	for (uint32_t i=0; i < _poly; ++i)
		buffer(i)->prepare_read(context.nframes());

	//cerr << path() << " output post: buffer: " << buffer(0) << endl;

	broadcast(context);
}


} // namespace Ingen
