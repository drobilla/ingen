/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef BREADCRUMBBOX_H
#define BREADCRUMBBOX_H

#include <list>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include "util/Path.h"

namespace Ingenuity {

class PatchController;
class BreadCrumb;


/** Collection of breadcrumb buttons forming a path.
 *
 * \ingroup Ingenuity
 */
class BreadCrumbBox : public Gtk::HBox
{
public:
	BreadCrumbBox();
	
	void build(Path path);

	sigc::signal<void, const Path&> signal_patch_selected;

private:
	void breadcrumb_clicked(BreadCrumb* crumb);

	Path                   _active_path;
	Path                   _full_path;
	bool                   _enable_signal;
	std::list<BreadCrumb*> _breadcrumbs;
};

} // namespace Ingenuity

#endif // BREADCRUMBBOX_H
