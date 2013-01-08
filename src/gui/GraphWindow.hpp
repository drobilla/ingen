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

#ifndef INGEN_GUI_GRAPH_WINDOW_HPP
#define INGEN_GUI_GRAPH_WINDOW_HPP

#include <string>

#include <gtkmm/builder.h>

#include "raul/SharedPtr.hpp"

#include "GraphBox.hpp"
#include "Window.hpp"

namespace Ingen {

namespace Client {
	class GraphModel;
}

namespace GUI {

/** A window for a graph.
 *
 * \ingroup GUI
 */
class GraphWindow : public Window
{
public:
	GraphWindow(BaseObjectType*                   cobject,
	            const Glib::RefPtr<Gtk::Builder>& xml);

	~GraphWindow();

	void init_window(App& app);

	SharedPtr<const Client::GraphModel> graph() const { return _box->graph(); }
	GraphBox*                           box()   const { return _box; }

	bool documentation_is_visible() { return _box->documentation_is_visible(); }

	void set_documentation(const std::string& doc, bool html) {
		_box->set_documentation(doc, html);
	}

	void show_port_status(const Client::PortModel* model,
	                      const Raul::Atom&        value) {
		_box->show_port_status(model, value);
	}

protected:
	void on_hide();
	void on_show();

private:
	GraphBox* _box;
	bool      _position_stored;
	int       _x;
	int       _y;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_GRAPH_WINDOW_HPP
