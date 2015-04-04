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

#include <string>

#include "ingen/Interface.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "NewSubgraphWindow.hpp"
#include "GraphView.hpp"

using namespace std;

namespace Ingen {
namespace GUI {

NewSubgraphWindow::NewSubgraphWindow(BaseObjectType*                   cobject,
                                     const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
{
	xml->get_widget("new_subgraph_name_entry", _name_entry);
	xml->get_widget("new_subgraph_message_label", _message_label);
	xml->get_widget("new_subgraph_polyphony_spinbutton", _poly_spinbutton);
	xml->get_widget("new_subgraph_ok_button", _ok_button);
	xml->get_widget("new_subgraph_cancel_button", _cancel_button);

	_name_entry->signal_changed().connect(sigc::mem_fun(this, &NewSubgraphWindow::name_changed));
	_ok_button->signal_clicked().connect(sigc::mem_fun(this, &NewSubgraphWindow::ok_clicked));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &NewSubgraphWindow::cancel_clicked));

	_ok_button->property_sensitive() = false;

	_poly_spinbutton->get_adjustment()->configure(1.0, 1.0, 128, 1.0, 10.0, 0);
}

void
NewSubgraphWindow::present(SPtr<const Client::GraphModel> graph,
                           Node::Properties               data)
{
	set_graph(graph);
	_initial_data = data;
	Gtk::Window::present();
}

/** Sets the graph controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
NewSubgraphWindow::set_graph(SPtr<const Client::GraphModel> graph)
{
	_graph = graph;
}

/** Called every time the user types into the name input box.
 * Used to display warning messages, and enable/disable the OK button.
 */
void
NewSubgraphWindow::name_changed()
{
	string name = _name_entry->get_text();
	if (!Raul::Symbol::is_valid(name)) {
		_message_label->set_text("Name contains invalid characters.");
		_ok_button->property_sensitive() = false;
	} else if (_app->store()->find(_graph->path().child(Raul::Symbol(name)))
	           != _app->store()->end()) {
		_message_label->set_text("An object already exists with that name.");
		_ok_button->property_sensitive() = false;
	} else {
		_message_label->set_text("");
		_ok_button->property_sensitive() = true;
	}
}
void
NewSubgraphWindow::ok_clicked()
{
	const uint32_t   poly = _poly_spinbutton->get_value_as_int();
	const Raul::Path path = _graph->path().child(
		Raul::Symbol::symbolify(_name_entry->get_text()));

	// Create graph
	Resource::Properties props;
	props.insert(make_pair(_app->uris().rdf_type,        Resource::Property(_app->uris().ingen_Graph)));
	props.insert(make_pair(_app->uris().ingen_polyphony, _app->forge().make(int32_t(poly))));
	props.insert(make_pair(_app->uris().ingen_enabled,   _app->forge().make(bool(true))));
	_app->interface()->put(
		Node::path_to_uri(path), props, Resource::Graph::INTERNAL);

	// Set external (block perspective) properties
	props = _initial_data;
	props.insert(make_pair(_app->uris().rdf_type,
	                       Resource::Property(_app->uris().ingen_Graph)));
	_app->interface()->put(
		Node::path_to_uri(path), _initial_data, Resource::Graph::EXTERNAL);

	hide();
}

void
NewSubgraphWindow::cancel_clicked()
{
	hide();
}

} // namespace GUI
} // namespace Ingen
