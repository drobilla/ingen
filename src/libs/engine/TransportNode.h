/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef TRANSPORTNODE_H
#define TRANSPORTNODE_H


#include <string>
#include <jack/transport.h>
#include "InternalNode.h"

namespace Om {

using std::string;


/** Transport Node, brings timing information into patches.
 *
 * This node uses the Jack transport API to get information about BPM, time
 * signature, etc.. all sample accurate.  Using this you can do
 * tempo-synced effects or even synthesis, etc.
 */
class TransportNode : public InternalNode
{
public:
	TransportNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);

	void run(size_t nframes);
};


} // namespace Om

#endif // TRANSPORTNODE_H
