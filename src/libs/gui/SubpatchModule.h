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


#ifndef SUBPATCHMODULE_H
#define SUBPATCHMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#include <raul/SharedPtr.h>
#include "client/PatchModel.h"
#include "PatchPortModule.h"
#include "NodeModule.h"
using std::string; using std::list;

namespace Ingen { namespace Client {
	class PatchModel;
	class NodeModel;
	class PortModel;
	class PatchWindow;
} }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {
	
class PatchCanvas;
class NodeControlWindow;


/** A module to represent a subpatch
 *
 * \ingroup GUI
 */
class SubpatchModule : public NodeModule
{
public:
	SubpatchModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PatchModel> controller);
	virtual ~SubpatchModule() {}

	void on_double_click(GdkEventButton* ev);

	void show_dialog();
	void browse_to_patch();
	void menu_remove();

	SharedPtr<PatchModel> patch() { return _patch; }

protected:
	SharedPtr<PatchModel> _patch;
};


} // namespace GUI
} // namespace Ingen

#endif // SUBPATCHMODULE_H
