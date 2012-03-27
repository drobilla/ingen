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

#ifndef INGEN_GUI_PATCHVIEW_HPP
#define INGEN_GUI_PATCHVIEW_HPP

#include <string>

#include <gtkmm.h>

#include "raul/SharedPtr.hpp"
#include "raul/URI.hpp"

namespace Raul { class Atom; }
namespace Ganv { class Port; class Item; }

namespace Ingen {

namespace Client {
	class PortModel;
	class MetadataModel;
	class PatchModel;
	class ObjectModel;
}
using namespace Ingen::Client;

namespace GUI {

class App;
class LoadPluginWindow;
class NewSubpatchWindow;
class NodeControlWindow;
class PatchCanvas;
class PatchDescriptionWindow;
class SubpatchModule;

/** The patch specific contents of a PatchWindow (ie the canvas and whatever else).
 *
 * \ingroup GUI
 */
class PatchView : public Gtk::Box
{
public:
	PatchView(BaseObjectType*                   cobject,
	          const Glib::RefPtr<Gtk::Builder>& xml);

	void init(App& app);

	SharedPtr<PatchCanvas>       canvas()               const { return _canvas; }
	SharedPtr<const PatchModel>  patch()                const { return _patch; }
	Gtk::ToolItem*               breadcrumb_container() const { return _breadcrumb_container; }

	void set_editable(bool editable);

	static SharedPtr<PatchView> create(App& app, SharedPtr<const PatchModel> patch);

	sigc::signal<void, const ObjectModel*> signal_object_entered;
	sigc::signal<void, const ObjectModel*> signal_object_left;

private:
	void set_patch(SharedPtr<const PatchModel> patch);

	void process_toggled();
	void poly_changed();
	void clear_clicked();
	void refresh_clicked();
	void on_editable_sig(bool locked);
	void editable_toggled();

	#if 0
	void canvas_item_entered(Gnome::Canvas::Item* item);
	void canvas_item_left(Gnome::Canvas::Item* item);
	#endif

	void property_changed(const Raul::URI& predicate, const Raul::Atom& value);

	void zoom_full();

	App* _app;

	SharedPtr<const PatchModel> _patch;
	SharedPtr<PatchCanvas>      _canvas;

	Gtk::ScrolledWindow*   _canvas_scrolledwindow;
	Gtk::Toolbar*          _toolbar;
	Gtk::ToggleToolButton* _process_but;
	Gtk::SpinButton*       _poly_spin;
	Gtk::ToolButton*       _refresh_but;
	Gtk::ToolButton*       _save_but;
	Gtk::ToolButton*       _zoom_normal_but;
	Gtk::ToolButton*       _zoom_full_but;
	Gtk::ToggleToolButton* _edit_mode_but;
	Gtk::ToolItem*         _breadcrumb_container;

	bool _enable_signal;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PATCHVIEW_HPP
