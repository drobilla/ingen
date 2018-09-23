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

#ifndef INGEN_GUI_GRAPH_WINDOW_HPP
#define INGEN_GUI_GRAPH_WINDOW_HPP

#include <string>

#include <gtkmm/builder.h>

#include "ingen/types.hpp"

#include "GraphBox.hpp"
#include "Window.hpp"

namespace ingen {

namespace client {
class GraphModel;
}

namespace gui {

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

	void init_window(App& app) override;

	SPtr<const client::GraphModel> graph() const { return _box->graph(); }
	GraphBox*                      box()   const { return _box; }

	bool documentation_is_visible() { return _box->documentation_is_visible(); }

	void set_documentation(const std::string& doc, bool html) {
		_box->set_documentation(doc, html);
	}

	void show_port_status(const client::PortModel* model,
	                      const Atom&              value) {
		_box->show_port_status(model, value);
	}

protected:
	void on_hide() override;
	void on_show() override;
	bool on_key_press_event(GdkEventKey* event) override;

private:
	GraphBox* _box;
	bool      _position_stored;
	int       _x;
	int       _y;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_GRAPH_WINDOW_HPP
