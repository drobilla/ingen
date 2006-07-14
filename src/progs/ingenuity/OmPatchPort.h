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

#ifndef OMPATCHPORT_H
#define OMPATCHPORT_H

#include <cassert>
#include <string>
#include <flowcanvas/Port.h>
#include "util/CountedPtr.h"

namespace LibOmClient { class PortModel; }
using namespace LibOmClient;
using namespace LibFlowCanvas;
using std::string; using std::list;

namespace OmGtk {
	
class FlowCanvas;
class PatchController;
class PatchWindow;
class OmPortModule;


/** A Port (on a pseudo node) in a patch canvas, to represent a port on that patch.
 * 
 * \ingroup OmGtk
 */
class OmPatchPort : public LibFlowCanvas::Port
{
public:
	OmPatchPort(OmPortModule* module, CountedPtr<PortModel> pm);

	virtual ~OmPatchPort() {}

	//void set_name(const string& n);
	
	CountedPtr<PortModel> model() const { return m_port_model; }
	
private:
	CountedPtr<PortModel> m_port_model;
};


} // namespace OmGtk

#endif // OMPATCHPORT_H
