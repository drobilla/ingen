/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_CONTROLPANEL_HPP
#define INGEN_GUI_CONTROLPANEL_HPP

#include <vector>
#include <string>
#include <utility> // for pair<>
#include <sigc++/sigc++.h>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "Controls.hpp"

namespace Raul { class Path; }

namespace Ingen {

namespace Client {
	class PortModel;
	class NodeModel;
}
using namespace Ingen::Client;

namespace GUI {


/** A group of controls for a node (or patch).
 *
 * Used by both NodeControlWindow and the main window (for patch controls).
 *
 * \ingroup GUI
 */
class ControlPanel : public Gtk::HBox {
public:
	ControlPanel(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	virtual ~ControlPanel();

	void init(SharedPtr<NodeModel> node, uint32_t poly);

	Control* find_port(const Raul::Path& path) const;

	void add_port(SharedPtr<PortModel> port);
	void remove_port(const Raul::Path& path);

	void enable_port(const Raul::Path& path);
	void disable_port(const Raul::Path& path);

	size_t             num_controls() const { return _controls.size(); }
	std::pair<int,int> ideal_size()   const { return _ideal_size; }

	// Callback for Control
	void value_changed_atom(SharedPtr<PortModel> port, const Raul::Atom& val);

	template <typename T>
	void value_changed(SharedPtr<PortModel> port, T val) {
		this->value_changed_atom(port, Raul::Atom(val));
	}

private:
	bool _callback_enabled;

	std::pair<int,int> _ideal_size;

	std::vector<Control*> _controls;
	Gtk::VBox*            _control_box;
};


} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_CONTROLPANEL_HPP
