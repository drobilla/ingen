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

#include "WindowFactory.hpp"
#include "App.hpp"
#include "PatchWindow.hpp"
#include "GladeFactory.hpp"
#include "NodePropertiesWindow.hpp"
#include "PatchPropertiesWindow.hpp"
#include "NodeControlWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "LoadPatchWindow.hpp"
#include "LoadRemotePatchWindow.hpp"
#include "UploadPatchWindow.hpp"
#include "LoadSubpatchWindow.hpp"
#include "RenameWindow.hpp"
#include "NewSubpatchWindow.hpp"

namespace Ingen {
namespace GUI {


WindowFactory::WindowFactory()
	: _load_plugin_win(NULL)
	, _load_patch_win(NULL)
	, _load_remote_patch_win(NULL)
	, _upload_patch_win(NULL)
	, _new_subpatch_win(NULL)
	, _load_subpatch_win(NULL)
	, _node_properties_win(NULL)
	, _patch_properties_win(NULL)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();

	xml->get_widget_derived("load_plugin_win", _load_plugin_win);
	xml->get_widget_derived("load_patch_win", _load_patch_win);
	xml->get_widget_derived("load_remote_patch_win", _load_remote_patch_win);
	xml->get_widget_derived("upload_patch_win", _upload_patch_win);
	xml->get_widget_derived("new_subpatch_win", _new_subpatch_win);
	xml->get_widget_derived("load_subpatch_win", _load_subpatch_win);
	xml->get_widget_derived("node_properties_win", _node_properties_win);
	xml->get_widget_derived("patch_properties_win", _patch_properties_win);
	xml->get_widget_derived("rename_win", _rename_win);
}


WindowFactory::~WindowFactory()
{
	for (PatchWindowMap::iterator i = _patch_windows.begin(); i != _patch_windows.end(); ++i)
		delete i->second;
	
	for (ControlWindowMap::iterator i = _control_windows.begin(); i != _control_windows.end(); ++i)
		delete i->second;
	
}


void
WindowFactory::clear()
{
	for (PatchWindowMap::iterator i = _patch_windows.begin(); i != _patch_windows.end(); ++i)
		delete i->second;
	
	_patch_windows.clear();
	
	for (ControlWindowMap::iterator i = _control_windows.begin(); i != _control_windows.end(); ++i)
		delete i->second;
	
	_control_windows.clear();
}


/** Returns the number of Patch windows currently visible.
 */
size_t
WindowFactory::num_open_patch_windows()
{
	size_t ret = 0;
	for (PatchWindowMap::iterator i = _patch_windows.begin(); i != _patch_windows.end(); ++i)
		if (i->second->is_visible())
			++ret;

	return ret;
}



PatchWindow*
WindowFactory::patch_window(SharedPtr<PatchModel> patch)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	return (w == _patch_windows.end()) ? NULL : w->second;
}

	
NodeControlWindow*
WindowFactory::control_window(SharedPtr<NodeModel> node)
{
	ControlWindowMap::iterator w = _control_windows.find(node->path());

	return (w == _control_windows.end()) ? NULL : w->second;
}


/** Present a PatchWindow for a Patch.
 *
 * If @a preferred is not NULL, it will be set to display @a patch if the patch
 * does not already have a visible window, otherwise that window will be presented and
 * @a preferred left unmodified.
 */
void
WindowFactory::present_patch(SharedPtr<PatchModel> patch, PatchWindow* preferred, SharedPtr<PatchView> view)
{
	assert( !view || view->patch() == patch);

	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end()) {
		(*w).second->present();
	} else if (preferred) {
		w = _patch_windows.find(preferred->patch()->path());
		assert((*w).second == preferred);

		preferred->set_patch(patch, view);
		_patch_windows.erase(w);
		_patch_windows[patch->path()] = preferred;
		preferred->present();

	} else {
		PatchWindow* win = new_patch_window(patch, view);
		win->present();
	}
}


PatchWindow*
WindowFactory::new_patch_window(SharedPtr<PatchModel> patch, SharedPtr<PatchView> view)
{
	assert( !view || view->patch() == patch);

	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference("patch_win");

	PatchWindow* win = NULL;
	xml->get_widget_derived("patch_win", win);
	assert(win);
	
	win->set_patch(patch, view);
	_patch_windows[patch->path()] = win;

	win->signal_delete_event().connect(sigc::bind<0>(
		sigc::mem_fun(this, &WindowFactory::remove_patch_window), win));

	return win;
}


bool
WindowFactory::remove_patch_window(PatchWindow* win, GdkEventAny* ignored)
{
	if (_patch_windows.size() <= 1) {
		Gtk::MessageDialog d(*win, "This is the last remaining open patch "
			"window.  Closing this window will exit Ingenuity (the engine will "
			"remain running).\n\nAre you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
			d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		int ret = d.run();
		if (ret == Gtk::RESPONSE_CLOSE)
			App::instance().quit();
		else
			return true;
	}

	PatchWindowMap::iterator w = _patch_windows.find(win->patch()->path());

	assert((*w).second == win);
	_patch_windows.erase(w);

	delete win;
	
	return false;
}


void
WindowFactory::present_controls(SharedPtr<NodeModel> node)
{
	NodeControlWindow* win = control_window(node);

	if (win) {
		win->present();
	} else {
		win = new_control_window(node);
		win->present();
	}
}


NodeControlWindow*
WindowFactory::new_control_window(SharedPtr<NodeModel> node)
{
	size_t poly = 1;
	if (node->polyphonic())
		poly = ((PatchModel*)node->parent().get())->poly();

	NodeControlWindow* win = new NodeControlWindow(node, poly);

	_control_windows[node->path()] = win;

	win->signal_delete_event().connect(sigc::bind<0>(
		sigc::mem_fun(this, &WindowFactory::remove_control_window), win));

	return win;
}


bool
WindowFactory::remove_control_window(NodeControlWindow* win, GdkEventAny* ignored)
{
	ControlWindowMap::iterator w = _control_windows.find(win->node()->path());

	assert((*w).second == win);
	_control_windows.erase(w);

	delete win;
	
	return true;
}


void
WindowFactory::present_load_plugin(SharedPtr<PatchModel> patch, MetadataMap data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_plugin_win->set_transient_for(*w->second);

	_load_plugin_win->present(patch, data);
}


void
WindowFactory::present_load_patch(SharedPtr<PatchModel> patch, MetadataMap data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_patch_win->set_transient_for(*w->second);

	_load_patch_win->set_merge(); // Import is the only choice

	_load_patch_win->present(patch, data);
}

	
void
WindowFactory::present_load_remote_patch(SharedPtr<PatchModel> patch, MetadataMap data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_remote_patch_win->set_transient_for(*w->second);

	_load_remote_patch_win->set_merge(); // Import is the only choice

	_load_remote_patch_win->present(patch, data);
}


void
WindowFactory::present_upload_patch(SharedPtr<PatchModel> patch)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_upload_patch_win->set_transient_for(*w->second);

	_upload_patch_win->present(patch);
}


void
WindowFactory::present_new_subpatch(SharedPtr<PatchModel> patch, MetadataMap data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_new_subpatch_win->set_transient_for(*w->second);

	_new_subpatch_win->present(patch, data);
}


void
WindowFactory::present_load_subpatch(SharedPtr<PatchModel> patch, MetadataMap data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_subpatch_win->set_transient_for(*w->second);
	
	_load_subpatch_win->present(patch, data);
}


void
WindowFactory::present_rename(SharedPtr<ObjectModel> object)
{
	PatchWindowMap::iterator w = _patch_windows.find(object->path());

	if (w != _patch_windows.end())
		_rename_win->set_transient_for(*w->second);
	
	_rename_win->present(object);
}


void
WindowFactory::present_properties(SharedPtr<NodeModel> node)
{
	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(node);
	if (patch) {
	
		PatchWindowMap::iterator w = _patch_windows.find(patch->path());
		if (w != _patch_windows.end())
			_patch_properties_win->set_transient_for(*w->second);

		_patch_properties_win->present(patch);
	
	} else {

		PatchWindowMap::iterator w = _patch_windows.find(node->parent()->path());
		if (w != _patch_windows.end())
			_node_properties_win->set_transient_for(*w->second);

		_node_properties_win->present(node);
	}
}


} // namespace GUI
} // namespace Ingen
