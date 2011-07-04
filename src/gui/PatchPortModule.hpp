/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_PATCHPORTMODULE_HPP
#define INGEN_GUI_PATCHPORTMODULE_HPP

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <libgnomecanvasmm.h>
#include "flowcanvas/Module.hpp"
#include "raul/URI.hpp"
#include "Port.hpp"

namespace Raul { class Atom; }

namespace Ingen { namespace Client {
	class PortModel;
	class NodeModel;
} }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {

class PatchCanvas;
class Port;
class PortMenu;

/** A "module" to represent a patch's port on its own canvas.
 *
 * Translation: This is the nameless single port pseudo module thingy.
 *
 * \ingroup GUI
 */
class PatchPortModule : public FlowCanvas::Module
{
public:
	static PatchPortModule* create(
		PatchCanvas&               canvas,
		SharedPtr<const PortModel> model,
		bool                       human);

	virtual void store_location();
	void show_human_names(bool b);

	void set_name(const std::string& n);

	SharedPtr<const PortModel> port() const { return _model; }

protected:
	PatchPortModule(PatchCanvas&               canvas,
	                SharedPtr<const PortModel> model);

	bool show_menu(GdkEventButton* ev);
	void set_selected(bool b);

	void set_port(Port* port) { _port = port; }

	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);

	SharedPtr<const PortModel> _model;
	Port*                      _port;
	PortMenu*                  _menu;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PATCHPORTMODULE_HPP
