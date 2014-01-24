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

#ifndef INGEN_GUI_PORT_HPP
#define INGEN_GUI_PORT_HPP

#include <cassert>
#include <string>

#include <gtkmm/menu.h>

#include "ganv/Port.hpp"
#include "ingen/types.hpp"

namespace Raul {
class Atom;
class URI;
}

namespace Ingen {

namespace Client { class PortModel; }

namespace GUI {

class App;
class GraphBox;

/** A Port on an Module.
 *
 * \ingroup GUI
 */
class Port : public Ganv::Port
{
public:
	static Port* create(
		App&                          app,
		Ganv::Module&                 module,
		SPtr<const Client::PortModel> pm,
		bool                          human_name,
		bool                          flip = false);

	~Port();

	SPtr<const Client::PortModel> model() const { return _port_model.lock(); }

	bool show_menu(GdkEventButton* ev);
	void update_metadata();

	void value_changed(const Atom& value);
	void activity(const Atom& value);
	void disconnected_from(SPtr<Client::PortModel> port);

	bool on_selected(gboolean b);

private:
	Port(App&                          app,
	     Ganv::Module&                 module,
	     SPtr<const Client::PortModel> pm,
	     const std::string&            name,
	     bool                          flip = false);

	Gtk::Menu* build_enum_menu();
	Gtk::Menu* build_uri_menu();
	GraphBox* get_graph_box() const;

	void property_changed(const Raul::URI& key, const Atom& value);
	void property_removed(const Raul::URI& key, const Atom& value);
	void moved();

	void on_value_changed(double value);
	void on_scale_point_activated(float f);
	void on_uri_activated(const Raul::URI& uri);
	bool on_event(GdkEvent* ev);
	void port_properties_changed();

	App&                          _app;
	WPtr<const Client::PortModel> _port_model;
	bool                          _entered : 1;
	bool                          _flipped : 1;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PORT_HPP
