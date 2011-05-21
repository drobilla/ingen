/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <string>
#include "ingen-config.h"
#include "client/PatchModel.hpp"
#include "App.hpp"
#include "WidgetFactory.hpp"
#include "LoadPatchWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "LoadRemotePatchWindow.hpp"
#include "NewSubpatchWindow.hpp"
#include "NodeControlWindow.hpp"
#include "PropertiesWindow.hpp"
#include "PatchView.hpp"
#include "PatchWindow.hpp"
#include "RenameWindow.hpp"
#include "WindowFactory.hpp"
#ifdef HAVE_CURL
#include "UploadPatchWindow.hpp"
#endif

using namespace std;

namespace Ingen {
namespace GUI {

WindowFactory::WindowFactory()
	: _load_plugin_win(NULL)
	, _load_patch_win(NULL)
	, _load_remote_patch_win(NULL)
	, _upload_patch_win(NULL)
	, _new_subpatch_win(NULL)
	, _properties_win(NULL)
{
	WidgetFactory::get_widget_derived("load_plugin_win", _load_plugin_win);
	WidgetFactory::get_widget_derived("load_patch_win", _load_patch_win);
	WidgetFactory::get_widget_derived("load_remote_patch_win", _load_remote_patch_win);
	WidgetFactory::get_widget_derived("new_subpatch_win", _new_subpatch_win);
	WidgetFactory::get_widget_derived("properties_win", _properties_win);
	WidgetFactory::get_widget_derived("rename_win", _rename_win);

#ifdef HAVE_CURL
	WidgetFactory::get_widget_derived("upload_patch_win", _upload_patch_win);
#endif
}

WindowFactory::~WindowFactory()
{
	for (PatchWindowMap::iterator i = _patch_windows.begin();
	     i != _patch_windows.end(); ++i)
		delete i->second;

	for (ControlWindowMap::iterator i = _control_windows.begin();
	     i != _control_windows.end(); ++i)
		delete i->second;

}

void
WindowFactory::clear()
{
	for (PatchWindowMap::iterator i = _patch_windows.begin();
	     i != _patch_windows.end(); ++i)
		delete i->second;

	_patch_windows.clear();

	for (ControlWindowMap::iterator i = _control_windows.begin();
	     i != _control_windows.end(); ++i)
		delete i->second;

	_control_windows.clear();
}

/** Returns the number of Patch windows currently visible.
 */
size_t
WindowFactory::num_open_patch_windows()
{
	size_t ret = 0;
	for (PatchWindowMap::iterator i = _patch_windows.begin();
	     i != _patch_windows.end(); ++i)
		if (i->second->is_visible())
			++ret;

	return ret;
}

PatchWindow*
WindowFactory::patch_window(SharedPtr<const PatchModel> patch)
{
	if (!patch)
		return NULL;

	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	return (w == _patch_windows.end()) ? NULL : w->second;
}

PatchWindow*
WindowFactory::parent_patch_window(SharedPtr<const NodeModel> node)
{
	if (!node)
		return NULL;

	return patch_window(PtrCast<PatchModel>(node->parent()));
}

NodeControlWindow*
WindowFactory::control_window(SharedPtr<const NodeModel> node)
{
	ControlWindowMap::iterator w = _control_windows.find(node->path());

	return (w == _control_windows.end()) ? NULL : w->second;
}

/** Present a PatchWindow for a Patch.
 *
 * If @a preferred is not NULL, it will be set to display @a patch if the patch
 * does not already have a visible window, otherwise that window will be
 * presented and @a preferred left unmodified.
 */
void
WindowFactory::present_patch(SharedPtr<const PatchModel> patch,
                             PatchWindow*                preferred,
                             SharedPtr<PatchView>        view)
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
WindowFactory::new_patch_window(SharedPtr<const PatchModel> patch,
                                SharedPtr<PatchView>        view)
{
	assert( !view || view->patch() == patch);

	PatchWindow* win = NULL;
	WidgetFactory::get_widget_derived("patch_win", win);
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
	if (_patch_windows.size() <= 1)
		return !App::instance().quit(*win);

	PatchWindowMap::iterator w = _patch_windows.find(win->patch()->path());

	assert((*w).second == win);
	_patch_windows.erase(w);

	delete win;

	return false;
}

void
WindowFactory::present_controls(SharedPtr<const NodeModel> node)
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
WindowFactory::new_control_window(SharedPtr<const NodeModel> node)
{
	uint32_t poly = 1;
	if (node->polyphonic() && node->parent())
		poly = ((PatchModel*)node->parent().get())->internal_poly();

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
WindowFactory::present_load_plugin(SharedPtr<const PatchModel> patch,
                                   GraphObject::Properties     data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_plugin_win->set_transient_for(*w->second);

	_load_plugin_win->set_modal(false);
	_load_plugin_win->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	int width, height;
	w->second->get_size(width, height);
	_load_plugin_win->set_default_size(width - width / 8, height / 2);
	_load_plugin_win->set_title(
		string("Load Plugin - ") + patch->path().chop_scheme() + " - Ingen");
	_load_plugin_win->present(patch, data);
}

void
WindowFactory::present_load_patch(SharedPtr<const PatchModel> patch,
                                  GraphObject::Properties     data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_patch_win->set_transient_for(*w->second);

	_load_patch_win->present(patch, true, data);
}

void
WindowFactory::present_load_subpatch(SharedPtr<const PatchModel> patch,
                                     GraphObject::Properties     data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_patch_win->set_transient_for(*w->second);

	_load_patch_win->present(patch, false, data);
}

void
WindowFactory::present_load_remote_patch(SharedPtr<const PatchModel> patch,
                                         GraphObject::Properties     data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_load_remote_patch_win->set_transient_for(*w->second);

	_load_remote_patch_win->present(patch, data);
}

void
WindowFactory::present_upload_patch(SharedPtr<const PatchModel> patch)
{
#ifdef HAVE_CURL
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_upload_patch_win->set_transient_for(*w->second);

	_upload_patch_win->present(patch);
#endif
}

void
WindowFactory::present_new_subpatch(SharedPtr<const PatchModel> patch,
                                    GraphObject::Properties     data)
{
	PatchWindowMap::iterator w = _patch_windows.find(patch->path());

	if (w != _patch_windows.end())
		_new_subpatch_win->set_transient_for(*w->second);

	_new_subpatch_win->present(patch, data);
}

void
WindowFactory::present_rename(SharedPtr<const ObjectModel> object)
{
	PatchWindowMap::iterator w = _patch_windows.find(object->path());
	if (w == _patch_windows.end())
		w = _patch_windows.find(object->path().parent());

	if (w != _patch_windows.end())
		_rename_win->set_transient_for(*w->second);

	_rename_win->present(object);
}

void
WindowFactory::present_properties(SharedPtr<const ObjectModel> object)
{
	PatchWindowMap::iterator w = _patch_windows.find(object->path());
	if (w == _patch_windows.end())
		w = _patch_windows.find(object->path().parent());
	if (w == _patch_windows.end())
		w = _patch_windows.find(object->path().parent().parent());

	if (w != _patch_windows.end())
		_properties_win->set_transient_for(*w->second);

	_properties_win->present(object);
}

} // namespace GUI
} // namespace Ingen
