/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef PATCHVIEW_H
#define PATCHVIEW_H

#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>
#include <libglademm.h>
#include <libgnomecanvasmm.h>
#include "raul/SharedPtr.hpp"

namespace Raul { class Atom; }
namespace FlowCanvas { class Port; class Item; }

namespace Ingen {

namespace Client {
	class PortModel;
	class MetadataModel;
	class PatchModel;
	class ObjectModel;
}
using namespace Ingen::Client;

namespace GUI {

class PatchCanvas;
class LoadPluginWindow;
class NewSubpatchWindow;
class LoadSubpatchWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PatchDescriptionWindow;
class SubpatchModule;


/** The patch specific contents of a PatchWindow (ie the canvas and whatever else).
 *
 * \ingroup GUI
 */
class PatchView : public Gtk::Box
{
public:
	PatchView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	~PatchView();

	SharedPtr<PatchCanvas> canvas()               const { return _canvas; }
	SharedPtr<PatchModel>  patch()                const { return _patch; }
	Gtk::ToolItem*         breadcrumb_container() const { return _breadcrumb_container; }

	void set_editable(bool editable);

	static SharedPtr<PatchView> create(SharedPtr<PatchModel> patch);

	sigc::signal<void, ObjectModel*> signal_object_entered;
	sigc::signal<void, ObjectModel*> signal_object_left;

private:
	void set_patch(SharedPtr<PatchModel> patch);

	void process_toggled();
	void poly_changed();
	void clear_clicked();
	void refresh_clicked();
	void on_editable_sig(bool locked);
	void editable_toggled();

	void canvas_item_entered(Gnome::Canvas::Item* item);
	void canvas_item_left(Gnome::Canvas::Item* item);

	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);

	void zoom_full();

	SharedPtr<PatchModel>  _patch;
	SharedPtr<PatchCanvas> _canvas;

	Gtk::ScrolledWindow* _canvas_scrolledwindow;

	Gtk::Toolbar*           _toolbar;
	Gtk::ToggleToolButton*  _process_but;
	Gtk::SpinButton*        _poly_spin;
	Gtk::ToolButton*        _clear_but;
	Gtk::ToolButton*        _refresh_but;
	Gtk::ToolButton*        _save_but;
	Gtk::ToolButton*        _zoom_normal_but;
	Gtk::ToolButton*        _zoom_full_but;
	Gtk::ToggleToolButton*  _edit_mode_but;
	Gtk::ToolItem*          _breadcrumb_container;

	bool _enable_signal;
};


} // namespace GUI
} // namespace Ingen

#endif // PATCHVIEW_H
