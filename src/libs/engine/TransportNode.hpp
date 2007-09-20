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

#ifndef TRANSPORTNODE_H
#define TRANSPORTNODE_H

#include <string>
#include <jack/transport.h>
#include "NodeBase.hpp"

namespace Ingen {


/** Transport Node, brings timing information into patches.
 *
 * This node uses the Jack transport API to get information about BPM, time
 * signature, etc.. all sample accurate.  Using this you can do
 * tempo-synced effects or even synthesis, etc.
 */
class TransportNode : public NodeBase
{
public:
	TransportNode(const std::string& path, bool polyphonic, Patch* parent, SampleRate srate, size_t buffer_size);

	virtual void process(SampleCount nframes, FrameTime start, FrameTime end);
};


} // namespace Ingen

#endif // TRANSPORTNODE_H
