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

#ifndef INGEN_GUI_WINDOWFACTORY_HPP
#define INGEN_GUI_WINDOWFACTORY_HPP

#include <map>

#include <gtkmm.h>

#include "ingen/GraphObject.hpp"
#include "raul/SharedPtr.hpp"

namespace Raul { class Path; }

namespace Ingen {

namespace Client {
class PatchModel;
class NodeModel;
class ObjectModel;
}

namespace GUI {

class App;
class LoadPatchWindow;
class LoadPluginWindow;
class NewSubpatchWindow;
class PropertiesWindow;
class PatchBox;
class PatchView;
class PatchWindow;
class RenameWindow;
class UploadPatchWindow;

/** Manager/Factory for all windows.
 *
 * This serves as a nice centralized spot for all window management issues,
 * as well as an enumeration of all windows (the goal being to reduce that
 * number as much as possible).
 */
class WindowFactory {
public:
	explicit WindowFactory(App& app);
	~WindowFactory();

	size_t num_open_patch_windows();

	PatchBox*    patch_box(SharedPtr<const Client::PatchModel> patch);
	PatchWindow* patch_window(SharedPtr<const Client::PatchModel> patch);
	PatchWindow* parent_patch_window(SharedPtr<const Client::NodeModel> node);

	void present_patch(
		SharedPtr<const Client::PatchModel> model,
		PatchWindow*                        preferred = NULL,
		SharedPtr<PatchView>                view      = SharedPtr<PatchView>());

	typedef GraphObject::Properties Properties;

	void present_load_plugin(SharedPtr<const Client::PatchModel> patch, Properties data=Properties());
	void present_load_patch(SharedPtr<const Client::PatchModel> patch, Properties data=Properties());
	void present_load_subpatch(SharedPtr<const Client::PatchModel> patch, Properties data=Properties());
	void present_new_subpatch(SharedPtr<const Client::PatchModel> patch, Properties data=Properties());
	void present_rename(SharedPtr<const Client::ObjectModel> object);
	void present_properties(SharedPtr<const Client::ObjectModel> object);

	bool remove_patch_window(PatchWindow* win, GdkEventAny* ignored = NULL);

	void set_main_box(PatchBox* box) { _main_box = box; }

	void clear();

private:
	typedef std::map<Raul::Path, PatchWindow*> PatchWindowMap;

	PatchWindow* new_patch_window(SharedPtr<const Client::PatchModel> patch,
	                              SharedPtr<PatchView>                view);

	App&               _app;
	PatchBox*          _main_box;
	PatchWindowMap     _patch_windows;
	LoadPluginWindow*  _load_plugin_win;
	LoadPatchWindow*   _load_patch_win;
	NewSubpatchWindow* _new_subpatch_win;
	PropertiesWindow*  _properties_win;
	RenameWindow*      _rename_win;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_WINDOWFACTORY_HPP
