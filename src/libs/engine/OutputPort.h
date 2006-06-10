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

#ifndef OUTPUTPORT_H
#define OUTPUTPORT_H

#include <string>
#include <cstdlib>
#include "PortBase.h"
#include "util/types.h"

namespace Om {

template <typename T> class InputPort;


/** An output port.
 *
 * Output ports always have a locally allocated buffer, and buffer() will
 * always return that buffer.  (This is very different from InputPort)
 *
 * This class actually adds no functionality to Port whatsoever right now,
 * it will in the future when more advanced port types exist, and it makes
 * things clearer throughout the engine.
 *
 * \ingroup engine
 */
template <typename T>
class OutputPort : public PortBase<T>
{
public:
	OutputPort(Node* node, const string& name, size_t index, size_t poly, PortInfo* port_info, size_t buffer_size);
	virtual ~OutputPort() {}

	void set_tied_port(InputPort<T>* port);

private:
	// Prevent copies (undefined)
	OutputPort(const OutputPort& copy);
	OutputPort<T>& operator=(const OutputPort<T>&);
	
	using PortBase<T>::m_is_tied;
	using PortBase<T>::m_tied_port;
};


template class OutputPort<sample>;

} // namespace Om

#endif // OUTPUTPORT_H
