/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef GLADEFACTORY_H
#define GLADEFACTORY_H

#include <string>
#include <libglademm/xml.h>

using std::string;

namespace OmGtk {


/** Creates glade references, so various objects can create widgets.
 * Purely static.
 *
 * \ingroup OmGtk
 */
class GladeFactory {
public:
	static Glib::RefPtr<Gnome::Glade::Xml>
	new_glade_reference(const string& toplevel_widget = "");

private:
	GladeFactory() {}

	static void find_glade_file();
	static Glib::ustring glade_filename;
};


} // namespace OmGtk

#endif // GLADEFACTORY_H
