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
#include "App.h"
#include "Configuration.h"
#include "PatchModel.h"
#include "ModelEngineInterface.h"
#include "ThreadedLoader.h"

using boost::optional;

namespace Ingenuity {


LoadRemotePatchWindow::LoadRemotePatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Dialog(cobject),
  _replace(true)
{
	xml->get_widget("load_remote_patch_uri_entry", _uri_entry);
	xml->get_widget("load_remote_patch_cancel_button", _cancel_button);
	xml->get_widget("load_remote_patch_open_button", _open_button);
	
	_open_button->signal_clicked().connect(sigc::mem_fun(this, &LoadRemotePatchWindow::open_clicked));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &LoadRemotePatchWindow::cancel_clicked));
}


void
LoadRemotePatchWindow::present(SharedPtr<PatchModel> patch, MetadataMap data)
{
	set_patch(patch);
	_initial_data = data;
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


} // namespace Ingenuity
