/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include <cassert>
#include <string>
#include <flowcanvas/Port.h>
#include "util/CountedPtr.h"

namespace Ingen { namespace Client { class PortModel; } }
using Ingen::Client::PortModel;

namespace Ingenuity {

class NodeModule;


/** A Port on an Module.
 * 
 * \ingroup Ingenuity
 */
class Port : public LibFlowCanvas::Port
{
public:
	Port(NodeModule* module, CountedPtr<PortModel> pm);

	virtual ~Port() {}

	CountedPtr<PortModel> model() const { return m_port_model; }
	
private:
	CountedPtr<PortModel> m_port_model;
};


} // namespace Ingenuity

#endif // PORT_H
