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

#ifndef WINDOW_FACTORY_H
#define WINDOW_FACTORY_H

#include <map>
#include <gtkmm.h>
#include <raul/SharedPtr.hpp>
#include "interface/GraphObject.hpp"
#include "client/PatchModel.hpp"
#include "PatchView.hpp"

using Ingen::Client::PatchModel;
using namespace Ingen::Shared;

namespace Ingen {
namespace GUI {

class PatchWindow;
class NodeControlWindow;
class PatchPropertiesWindow;
class NodePropertiesWindow;
class PortPropertiesWindow;
class LoadPatchWindow;
class LoadRemotePatchWindow;
class UploadPatchWindow;
class RenameWindow;


/** Manager/Factory for all windows.
 *
 * This serves as a nice centralized spot for all window management issues,
 * as well as an enumeration of all windows (the goal being to reduce that
 * number as much as possible).
 */
class WindowFactory {
public:
	WindowFactory();
	~WindowFactory();

	size_t num_open_patch_windows();

	PatchWindow*       patch_window(SharedPtr<PatchModel> patch);
	NodeControlWindow* control_window(SharedPtr<NodeModel> node);

	void present_patch(SharedPtr<PatchModel> patch,
	                   PatchWindow*          preferred = NULL,
	                   SharedPtr<PatchView>  patch     = SharedPtr<PatchView>());

	void present_controls(SharedPtr<NodeModel> node);

	void present_load_plugin(SharedPtr<PatchModel> patch, GraphObject::Variables data=GraphObject::Variables());
	void present_load_patch(SharedPtr<PatchModel> patch, GraphObject::Variables data=GraphObject::Variables());
	void present_load_remote_patch(SharedPtr<PatchModel> patch, GraphObject::Variables data=GraphObject::Variables());
	void present_upload_patch(SharedPtr<PatchModel> patch);
	void present_new_subpatch(SharedPtr<PatchModel> patch, GraphObject::Variables data=GraphObject::Variables());
	void present_load_subpatch(SharedPtr<PatchModel> patch, GraphObject::Variables data=GraphObject::Variables());
	void present_rename(SharedPtr<ObjectModel> object);
	void present_properties(SharedPtr<ObjectModel> object);
	
	bool remove_patch_window(PatchWindow* win, GdkEventAny* ignored = NULL);

	void clear();

private:
	typedef std::map<Path, PatchWindow*>       PatchWindowMap;
	typedef std::map<Path, NodeControlWindow*> ControlWindowMap;

	PatchWindow* new_patch_window(SharedPtr<PatchModel> patch, SharedPtr<PatchView> view);


	NodeControlWindow* new_control_window(SharedPtr<NodeModel> node);
	bool               remove_control_window(NodeControlWindow* win, GdkEventAny* ignored);

	PatchWindowMap   _patch_windows;
	ControlWindowMap _control_windows;

	LoadPluginWindow*      _load_plugin_win;
	LoadPatchWindow*       _load_patch_win;
	LoadRemotePatchWindow* _load_remote_patch_win;
	UploadPatchWindow*     _upload_patch_win;
	NewSubpatchWindow*     _new_subpatch_win;
	LoadSubpatchWindow*    _load_subpatch_win;
	PatchPropertiesWindow* _patch_properties_win;
	NodePropertiesWindow*  _node_properties_win;
	PortPropertiesWindow*  _port_properties_win;
	RenameWindow*          _rename_win;
};


} // namespace GUI
} // namespace Ingen

#endif // WINDOW_FACTORY_H
