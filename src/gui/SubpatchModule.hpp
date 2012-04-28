/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_SUBPATCHMODULE_HPP
#define INGEN_GUI_SUBPATCHMODULE_HPP

#include <string>
#include "raul/SharedPtr.hpp"
#include "PatchPortModule.hpp"
#include "NodeModule.hpp"

namespace Ingen { namespace Client {
	class PatchModel;
	class NodeModel;
	class PortModel;
	class PatchWindow;
} }

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
	SubpatchModule(PatchCanvas&                        canvas,
	               SharedPtr<const Client::PatchModel> controller);

	virtual ~SubpatchModule() {}

	bool on_double_click(GdkEventButton* ev);

	void store_location(double x, double y);

	void browse_to_patch();
	void menu_remove();

	SharedPtr<const Client::PatchModel> patch() const { return _patch; }

protected:
	SharedPtr<const Client::PatchModel> _patch;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_SUBPATCHMODULE_HPP
