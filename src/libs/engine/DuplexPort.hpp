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

#ifndef DUPLEXPORT_H
#define DUPLEXPORT_H

#include <string>
#include <raul/Array.hpp>
#include "types.hpp"
#include "Buffer.hpp"
#include "InputPort.hpp"
#include "OutputPort.hpp"

namespace Ingen {
	
class Node;


/** A duplex port (which is both an InputPort and an OutputPort)
 *
 * This is used for Patch ports, since they need to appear as both an input
 * and an output port based on context.  Eg. a patch output appears as an
 * input inside the patch, so nodes inside the patch can feed it data.
 *
 * \ingroup engine
 */
class DuplexPort : public InputPort, public OutputPort
{
public:
	DuplexPort(Node* parent, const std::string& name, size_t index, size_t poly, DataType type, size_t buffer_size, bool is_output);
	virtual ~DuplexPort() {}

	void pre_process(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process(SampleCount nframes, FrameTime start, FrameTime end);
	
	bool is_input()  const { return !_is_output; }
	bool is_output() const { return _is_output; }

protected:
	bool _is_output;
};


} // namespace Ingen

#endif // DUPLEXPORT_H
