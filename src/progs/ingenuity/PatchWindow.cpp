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

#include "PatchWindow.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include "App.h"
#include "ModelEngineInterface.h"
#include "PatchCanvas.h"
#include "LoadPluginWindow.h"
#include "PatchModel.h"
#include "NewSubpatchWindow.h"
#include "LoadPatchWindow.h"
#include "LoadSubpatchWindow.h"
#include "NodeControlWindow.h"
#include "PatchPropertiesWindow.h"
#include "ConfigWindow.h"
#include "MessagesWindow.h"
#include "PatchTreeWindow.h"
#include "BreadCrumbBox.h"
#include "Store.h"
#include "ConnectWindow.h"
#include "ThreadedLoader.h"
#include "WindowFactory.h"
#include "PatchView.h"

namespace Ingenuity {


PatchWindow::PatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  _enable_signal(true),
  _position_stored(false),
  _x(0),
  _y(0),
  _breadcrumb_box(NULL)
{
	property_visible() = false;

	xml->get_widget("patch_win_vbox", _vbox);
	xml->get_widget("patch_win_viewport", _viewport);
	//xml->get_widget("patch_win_status_bar", _status_bar);
	//xml->get_widget("patch_open_menuitem", _menu_open);
	xml->get_widget("patch_import_menuitem", _menu_import);
	//xml->get_widget("patch_open_into_menuitem", _menu_open_into);
	xml->get_widget("patch_save_menuitem", _menu_save);
	xml->get_widget("patch_save_as_menuitem", _menu_save_as);
	xml->get_widget("patch_cut_menuitem", _menu_cut);
	xml->get_widget("patch_copy_menuitem", _menu_copy);
	xml->get_widget("patch_paste_menuitem", _menu_paste);
	xml->get_widget("patch_delete_menuitem", _menu_delete);
	xml->get_widget("patch_close_menuitem", _menu_close);
	xml->get_widget("patch_configuration_menuitem", _menu_configuration);
	xml->get_widget("patch_quit_menuitem", _menu_quit);
	xml->get_widget("patch_view_control_window_menuitem", _menu_view_control_window);
	xml->get_widget("patch_view_engine_window_menuitem", _menu_view_engine_window);
	xml->get_widget("patch_properties_menuitem", _menu_view_patch_properties);
	xml->get_widget("patch_fullscreen_menuitem", _menu_fullscreen);
	xml->get_widget("patch_arrange_menuitem", _menu_arrange);
	xml->get_widget("patch_clear_menuitem", _menu_clear);
	xml->get_widget("patch_destroy_menuitem", _menu_destroy_patch);
	xml->get_widget("patch_view_messages_window_menuitem", _menu_view_messages_window);
	xml->get_widget("patch_view_patch_tree_window_menuitem", _menu_view_patch_tree_window);
	xml->get_widget("patch_help_about_menuitem", _menu_help_about);

	_menu_view_control_window->property_sensitive() = false;
	//m_status_bar->push(App::instance().engine()->engine_url());
	//m_status_bar->pack_start(*Gtk::manage(new Gtk::Image(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_MENU)), false, false);
	
	/*_menu_open->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_open));*/
	_menu_import->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_import));
	_menu_save->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save));
	_menu_save_as->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save_as));
	_menu_copy->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_copy));
	_menu_delete->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_delete));
	_menu_quit->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_quit));
	_menu_configuration->signal_activate().connect(
		sigc::mem_fun(App::instance().configuration_dialog(), &ConfigWindow::show));
	_menu_fullscreen->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_fullscreen_toggled));
	_menu_arrange->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_arrange));
	_menu_view_engine_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_engine));
	_menu_view_control_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_controls));
	_menu_view_patch_properties->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_properties));
	_menu_destroy_patch->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_destroy));
	_menu_clear->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_clear));
	_menu_view_messages_window->signal_activate().connect(
		sigc::mem_fun<void>(App::instance().messages_dialog(), &MessagesWindow::present));
	_menu_view_patch_tree_window->signal_activate().connect(
		sigc::mem_fun<void>(App::instance().patch_tree(), &PatchTreeWindow::present));

	_menu_help_about->signal_activate().connect(
		sigc::mem_fun<void>(App::instance().about_dialog(), &Gtk::Dialog::present));
	
	_breadcrumb_box = new BreadCrumbBox();
	_breadcrumb_box->signal_patch_selected.connect(sigc::mem_fun(this, &PatchWindow::set_patch_from_path));
}


PatchWindow::~PatchWindow()
{
	// Prevents deletion
	//m_patch->claim_patch_view();

	delete _breadcrumb_box;
}


/** Set the patch controller from a Path (for use by eg. BreadCrumbBox)
 */
void 
PatchWindow::set_patch_from_path(const Path& path, SharedPtr<PatchView> view)
{	
	if (view) {
		assert(view->patch()->path() == path);
		App::instance().window_factory()->present_patch(view->patch(), this, view);
	} else {
		SharedPtr<PatchModel> model = PtrCast<PatchModel>(App::instance().store()->object(path));
		if (model)
			App::instance().window_factory()->present_patch(model, this);
	}
}


/** Sets the patch controller for this window and initializes everything.
 *
 * If @a view is NULL, a new view will be created.
 */
void
PatchWindow::set_patch(SharedPtr<PatchModel> patch, SharedPtr<PatchView> view)
{
	if (!patch || patch == _patch)
		return;

	_enable_signal = false;

	_patch = patch;

	_view = view;

	if (!_view)
		_view = _breadcrumb_box->view(patch->path());
	
	if (!_view)
		_view = PatchView::create(patch);
	
	assert(_view);

	// Add view to our viewport
	if (_view->get_parent())
		_view->get_parent()->remove(*_view.get());

	_viewport->remove();
	_viewport->add(*_view.get());


	if (_breadcrumb_box->get_parent())
		_breadcrumb_box->get_parent()->remove(*_breadcrumb_box);

	_view->breadcrumb_container()->remove();
	_view->breadcrumb_container()->add(*_breadcrumb_box);
	_view->breadcrumb_container()->show();

	_breadcrumb_box->build(patch->path(), _view);
	_breadcrumb_box->show();
	
	_menu_view_control_window->property_sensitive() = false;

	for (PortModelList::const_iterator p = patch->ports().begin();
			p != patch->ports().end(); ++p) {
		if ((*p)->is_control()) {
			_menu_view_control_window->property_sensitive() = true;
			break;
		}
	}

	int width, height;
	get_size(width, height);
	_view->canvas()->scroll_to(
			((int)_view->canvas()->width() - width)/2,
			((int)_view->canvas()->height() - height)/2);

	set_title(_patch->path() + " - Ingenuity");

	//m_properties_window->patch_model(pc->patch_model());

	if (patch->path() == "/")
		_menu_destroy_patch->set_sensitive(false);
	else
		_menu_destroy_patch->set_sensitive(true);
	
	show_all();

	_enable_signal = true;
}


void
PatchWindow::event_show_engine()
{
	if (_patch)
		App::instance().connect_window()->show();
}


void
PatchWindow::event_show_controls()
{
	App::instance().window_factory()->present_controls(_patch);
}


void
PatchWindow::event_show_properties()
{
	App::instance().window_factory()->present_properties(_patch);
}


void
PatchWindow::event_import()
{
	App::instance().window_factory()->present_load_patch(_patch);
}


void
PatchWindow::event_save()
{
	if (_patch->filename() == "")
		event_save_as();
	else
		App::instance().loader()->save_patch(_patch, _patch->filename(), false);
}


void
PatchWindow::event_save_as()
{
	Gtk::FileChooserDialog dialog(*this, "Save Patch", Gtk::FILE_CHOOSER_ACTION_SAVE);
	
	Gtk::VBox* box = dialog.get_vbox();
	Gtk::Label warning("Warning:  Recursively saving will overwrite any subpatch files \
		without confirmation.");
	box->pack_start(warning, false, false, 2);
	Gtk::CheckButton recursive_checkbutton("Recursively save all subpatches");
	box->pack_start(recursive_checkbutton, false, false, 0);
	recursive_checkbutton.show();
			
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);	
	
	// Set current folder to most sensible default
	const string& current_filename = _patch->filename();
	if (current_filename.length() > 0)
		dialog.set_filename(current_filename);
	else if (App::instance().configuration()->patch_folder().length() > 0)
		dialog.set_current_folder(App::instance().configuration()->patch_folder());
	
	int result = dialog.run();
	bool recursive = recursive_checkbutton.get_active();
	
	assert(result == Gtk::RESPONSE_OK || result == Gtk::RESPONSE_CANCEL || result == Gtk::RESPONSE_NONE);
	
	if (result == Gtk::RESPONSE_OK) {	
		string filename = dialog.get_filename();
		if (filename.length() < 7 || filename.substr(filename.length()-6) != ".ingen")
			filename += ".ingen";
			
		bool confirm = false;
		std::fstream fin;
		fin.open(filename.c_str(), std::ios::in);
		if (fin.is_open()) {  // File exists
			string msg = "File already exists!  Are you sure you want to overwrite ";
			msg += filename + "?";
			Gtk::MessageDialog confirm_dialog(*this,
				msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			if (confirm_dialog.run() == Gtk::RESPONSE_YES)
				confirm = true;
			else
				confirm = false;
		} else {  // File doesn't exist
			confirm = true;
		}
		fin.close();
		
		if (confirm) {
			App::instance().loader()->save_patch(_patch, filename, recursive);
			//m_patch->set_metadata("filename", Atom(filename.c_str()));
		}
	}
	App::instance().configuration()->set_patch_folder(dialog.get_current_folder());
}


void
PatchWindow::event_copy()
{
	if (_view)
		_view->canvas()->copy_selection();
}


void
PatchWindow::event_delete()
{
	if (_view)
		_view->canvas()->destroy_selection();
}


void
PatchWindow::on_show()
{
	if (_position_stored)
		move(_x, _y);

	Gtk::Window::on_show();
}


void
PatchWindow::on_hide()
{
	_position_stored = true;
	get_position(_x, _y);
	Gtk::Window::on_hide();
}


bool
PatchWindow::on_key_press_event(GdkEventKey* event)
{
	if (event->keyval == GDK_Delete) {
		cerr << "FIXME: delete key\n";
		/*
		if (_patch && _patch->get_view()) {
			assert(_patch->get_view()->canvas());
			_patch->get_view()->canvas()->destroy_selected();
		}*/
		return true;
	} else {
		return Gtk::Window::on_key_press_event(event);
	}
}


void
PatchWindow::event_quit()
{
	Gtk::MessageDialog d(*this, "Would you like to quit just Ingenuity\nor kill the engine as well?",
			true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
	d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	
	Gtk::Button* b = d.add_button(Gtk::Stock::REMOVE, 2); // kill
	b->set_label("_Kill Engine");
	Gtk::Widget* kill_img = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_BUTTON));
	b->set_image(*kill_img);

	b = d.add_button(Gtk::Stock::QUIT, 1); // just exit
	b->set_label("_Quit");
	Gtk::Widget* close_img = Gtk::manage(new Gtk::Image(Gtk::Stock::QUIT, Gtk::ICON_SIZE_BUTTON));
	b->set_image(*close_img);
	b->grab_default();
	
	int ret = d.run();
	if (ret == 1) {
		App::instance().quit();
	} else if (ret == 2) {
		App::instance().engine()->quit();
		App::instance().quit();
	}
	// Otherwise cancelled, do nothing
}


void
PatchWindow::event_destroy()
{
	App::instance().engine()->destroy(_patch->path());
}


void
PatchWindow::event_clear()
{
	App::instance().engine()->clear_patch(_patch->path());
}


void
PatchWindow::event_arrange()
{
	_view->canvas()->arrange();
}


void
PatchWindow::event_fullscreen_toggled()
{
	// FIXME: ugh, use GTK signals to track state and know for sure
	static bool is_fullscreen = false;

	if (!is_fullscreen) {
		fullscreen();
		is_fullscreen = true;
	} else {
		unfullscreen();
		is_fullscreen = false;
	}
}


} // namespace Ingenuity
