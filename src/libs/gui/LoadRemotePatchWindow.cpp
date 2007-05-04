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

#include "LoadRemotePatchWindow.h"
#include <sys/types.h>
#include <dirent.h>
#include <boost/optional/optional.hpp>
#include <raul/RDFQuery.h>
#include "interface/EngineInterface.h"
#include "client/PatchModel.h"
#include "App.h"
#include "Configuration.h"
#include "ThreadedLoader.h"

using boost::optional;
using namespace Raul;

namespace Ingen {
namespace GUI {


LoadRemotePatchWindow::LoadRemotePatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Dialog(cobject),
  _replace(true)
{
	xml->get_widget("load_remote_patch_treeview", _treeview);
	xml->get_widget("load_remote_patch_uri_entry", _uri_entry);
	xml->get_widget("load_remote_patch_cancel_button", _cancel_button);
	xml->get_widget("load_remote_patch_open_button", _open_button);
	
	_liststore = Gtk::ListStore::create(_columns);
	_treeview->set_model(_liststore);
	_treeview->append_column("Name", _columns._col_name);
	_treeview->append_column("URI", _columns._col_uri);
	
	_selection = _treeview->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(this, &LoadRemotePatchWindow::patch_selected));
	_treeview->signal_row_activated().connect(sigc::mem_fun(this, &LoadRemotePatchWindow::patch_activated));

	_open_button->signal_clicked().connect(sigc::mem_fun(this, &LoadRemotePatchWindow::open_clicked));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &LoadRemotePatchWindow::cancel_clicked));
	_uri_entry->signal_changed().connect(sigc::mem_fun(this, &LoadRemotePatchWindow::uri_changed));
}


void
LoadRemotePatchWindow::present(SharedPtr<PatchModel> patch, MetadataMap data)
{
	_liststore->clear();

	set_patch(patch);
	_initial_data = data;
	
	RDF::Model model(*App::instance().rdf_world(),
			"http://rdf.drobilla.net/ingen_patches/index.ttl",
			"http://rdf.drobilla.net/ingen_patches/");

	RDF::Query query(*App::instance().rdf_world(), Glib::ustring(
		"SELECT DISTINCT ?name ?uri WHERE {"
		"  ?uri a            ingen:Patch ;"
		"       doap:name    ?name ."
		"}"));

	RDF::Query::Results results = query.run(*App::instance().rdf_world(), model);
	
	for (RDF::Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Gtk::TreeModel::iterator iter = _liststore->append();
		(*iter)[_columns._col_name] = (*i)["name"].to_string();
		(*iter)[_columns._col_uri] = (*i)["uri"].to_string();
	}

	_treeview->columns_autosize();
	
	Gtk::Window::present();
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
LoadRemotePatchWindow::set_patch(SharedPtr<PatchModel> patch)
{
	_patch = patch;
}


void
LoadRemotePatchWindow::uri_changed()
{
	_open_button->property_sensitive() = (_uri_entry->get_text().length() > 0);
}


void
LoadRemotePatchWindow::patch_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col)
{
	open_clicked();
}


void
LoadRemotePatchWindow::patch_selected()
{
	Gtk::TreeModel::iterator selected_i = _selection->get_selected();
	
	if (selected_i) { // If anything is selected			
		const Glib::ustring uri = selected_i->get_value(_columns._col_uri);
		_uri_entry->set_text(uri);
	}
}


void
LoadRemotePatchWindow::open_clicked()
{
	Glib::ustring uri = _uri_entry->get_text();

	cerr << "OPEN URI: " << uri << endl;
	
	// If unset load_patch will load values
	optional<const string&> name;
	optional<size_t> poly;
	
	optional<Path> parent;
	
	if (_replace)
		App::instance().engine()->clear_patch(_patch->path());

	if (_patch->path() != "/")
		parent = _patch->path().parent();

	App::instance().loader()->load_patch(true, uri, "/",
		_initial_data, parent, name, poly);
	
	hide();
}			


void
LoadRemotePatchWindow::cancel_clicked()
{
	hide();
}


} // namespace GUI
} // namespace Ingen
