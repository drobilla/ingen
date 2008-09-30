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

#include "GladeFactory.hpp"
#include <iostream>
#include <fstream>

using namespace std;

namespace Ingen {
namespace GUI {


Glib::ustring GladeFactory::glade_filename = "";


void
GladeFactory::find_glade_file()
{
	char* env_path = getenv("INGEN_GLADE_PATH");
	if (env_path)
		glade_filename = env_path;
	else
		glade_filename = "./ingen_gui.glade";

	ifstream fs(glade_filename.c_str());
	if (fs.fail()) { // didn't find it, check INGEN_DATA_DIR
		fs.clear();
		glade_filename = INGEN_DATA_DIR;
		glade_filename += "/ingen_gui.glade";
	
		fs.open(glade_filename.c_str());
		if (fs.fail()) {
			cerr << "[GladeFactory] Unable to find ingen_gui.glade in current directory or " << INGEN_DATA_DIR << "." << endl;
			throw;
		}
		fs.close();
	}
	cerr << "[GladeFactory] Loading widgets from " << glade_filename.c_str() << endl;
}


Glib::RefPtr<Gnome::Glade::Xml>
GladeFactory::new_glade_reference(const string& toplevel_widget)
{
	if (glade_filename == "")
		find_glade_file();

	try {
		if (toplevel_widget == "")
			return Gnome::Glade::Xml::create(glade_filename);
		else
			return Gnome::Glade::Xml::create(glade_filename, toplevel_widget.c_str());
	} catch (const Gnome::Glade::XmlError& ex) {
		cerr << ex.what() << endl;
		throw ex;
	}
}


} // namespace GUI
} // namespace Ingen
