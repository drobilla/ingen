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
#include "util/CountedPtr.h"
#include "PatchView.h"

namespace Ingenuity {

class BreadCrumb;


/** Collection of breadcrumb buttons forming a path.
 *
 * This doubles as a cache for PatchViews.
 *
 * \ingroup Ingenuity
 */
class BreadCrumbBox : public Gtk::HBox
{
public:
	BreadCrumbBox();
	
	CountedPtr<PatchView> view(const Path& path);

	void build(Path path, CountedPtr<PatchView> view);

	sigc::signal<void, const Path&, CountedPtr<PatchView> > signal_patch_selected;

private:
	BreadCrumb* create_crumb(const Path&           path,
                             CountedPtr<PatchView> view = CountedPtr<PatchView>());

	void breadcrumb_clicked(BreadCrumb* crumb);
	
	void object_removed(const Path& path);
	void object_renamed(const Path& old_path, const Path& new_path);

	Path                   _active_path;
	Path                   _full_path;
	bool                   _enable_signal;
	std::list<BreadCrumb*> _breadcrumbs;
};

} // namespace Ingenuity

#endif // BREADCRUMBBOX_H
