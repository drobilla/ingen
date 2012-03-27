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

#ifndef INGEN_GUI_CONTROLPANEL_HPP
#define INGEN_GUI_CONTROLPANEL_HPP

#include <string>
#include <utility>
#include <vector>

#include <gtkmm.h>

#include "Controls.hpp"

namespace Raul { class Path; }

namespace Ingen {

namespace Client {
	class PortModel;
	class NodeModel;
}
using namespace Ingen::Client;

namespace GUI {

class App;

/** A group of controls for a node (or patch).
 *
 * Used by both NodeControlWindow and the main window (for patch controls).
 *
 * \ingroup GUI
 */
class ControlPanel : public Gtk::HBox {
public:
	ControlPanel(BaseObjectType*                   cobject,
	             const Glib::RefPtr<Gtk::Builder>& xml);
	virtual ~ControlPanel();

	void init(App& app, SharedPtr<const NodeModel> node, uint32_t poly);

	Control* find_port(const Raul::Path& path) const;

	void add_port(SharedPtr<const PortModel> port);
	void remove_port(const Raul::Path& path);

	void enable_port(const Raul::Path& path);
	void disable_port(const Raul::Path& path);

	size_t             num_controls() const { return _controls.size(); }
	std::pair<int,int> ideal_size()   const { return _ideal_size; }

	// Callback for Control
	void value_changed_atom(SharedPtr<const PortModel> port, const Raul::Atom& val);

	template <typename T>
	void value_changed(SharedPtr<const PortModel> port, T val) {
		this->value_changed_atom(port, _app->forge().make(val));
	}

private:
	App*                  _app;
	std::pair<int,int>    _ideal_size;
	std::vector<Control*> _controls;
	Gtk::VBox*            _control_box;
	bool                  _callback_enabled;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_CONTROLPANEL_HPP
