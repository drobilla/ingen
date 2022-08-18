/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include "ganv/Port.hpp"

#include <gdk/gdk.h>
#include <glib.h>

#include <memory>
#include <string>

namespace Ganv {
class Module;
} // namespace Ganv

namespace Gtk {
class Menu;
} // namespace Gtk

namespace ingen {

class URI;
class Atom;

namespace client {
class PortModel;
} // namespace client

namespace gui {

class App;
class GraphBox;

/** A Port on an Module.
 *
 * \ingroup GUI
 */
class Port : public Ganv::Port
{
public:
	static Port* create(App&                                            app,
	                    Ganv::Module&                                   module,
	                    const std::shared_ptr<const client::PortModel>& pm,
	                    bool flip = false);

	~Port() override;

	std::shared_ptr<const client::PortModel> model() const
	{
		return _port_model.lock();
	}

	bool show_menu(GdkEventButton* ev);
	void update_metadata();
	void ensure_label();

	void value_changed(const Atom& value);
	void activity(const Atom& value);

	bool on_selected(gboolean b) override;

private:
	Port(App&                                            app,
	     Ganv::Module&                                   module,
	     const std::shared_ptr<const client::PortModel>& pm,
	     const std::string&                              name,
	     bool                                            flip = false);

	static std::string
	port_label(App& app, const std::shared_ptr<const client::PortModel>& pm);

	Gtk::Menu* build_enum_menu();
	Gtk::Menu* build_uri_menu();
	GraphBox* get_graph_box() const;

	void property_changed(const URI& key, const Atom& value);
	void property_removed(const URI& key, const Atom& value);
	void moved();

	void on_value_changed(double value);
	void on_scale_point_activated(float f);
	void on_uri_activated(const URI& uri);
	bool on_event(GdkEvent* ev) override;
	void port_properties_changed();
	void set_type_tag();

	App&                                   _app;
	std::weak_ptr<const client::PortModel> _port_model;
	bool                                   _entered : 1;
	bool                                   _flipped : 1;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_PORT_HPP
