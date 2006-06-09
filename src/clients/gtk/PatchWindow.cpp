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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PatchWindow.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include "App.h"
#include "PatchView.h"
#include "OmFlowCanvas.h"
#include "PatchController.h"
#include "LoadPluginWindow.h"
#include "PatchModel.h"
#include "NewSubpatchWindow.h"
#include "LoadPatchWindow.h"
#include "LoadSubpatchWindow.h"
#include "NodeControlWindow.h"
#include "PatchDescriptionWindow.h"
#include "ConfigWindow.h"
#include "MessagesWindow.h"
#include "PatchTreeWindow.h"
#include "Controller.h"
#include "BreadCrumb.h"
#include "Store.h"

namespace OmGtk {


PatchWindow::PatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  m_patch(NULL),
  m_load_plugin_window(NULL),
  m_new_subpatch_window(NULL),
  m_enable_signal(true),
  m_position_stored(false),
  m_x(0),
  m_y(0)
{
	property_visible() = false;

	xml->get_widget("patch_win_vbox", m_vbox);
	xml->get_widget("patch_win_viewport", m_viewport);
	xml->get_widget("patch_win_breadcrumb_box", m_breadcrumb_box);
	//xml->get_widget("patch_win_status_bar", m_status_bar);
	xml->get_widget("patch_open_menuitem", m_menu_open);
	xml->get_widget("patch_open_into_menuitem", m_menu_open_into);
	xml->get_widget("patch_save_menuitem", m_menu_save);
	xml->get_widget("patch_save_as_menuitem", m_menu_save_as);
	xml->get_widget("patch_close_menuitem", m_menu_close);
	xml->get_widget("patch_configuration_menuitem", m_menu_configuration);
	xml->get_widget("patch_quit_menuitem", m_menu_quit);
	xml->get_widget("patch_quit_and_kill_menuitem", m_menu_quit_and_kill);
	xml->get_widget("patch_view_control_window_menuitem", m_menu_view_control_window);
	xml->get_widget("patch_description_menuitem", m_menu_view_patch_description);
	xml->get_widget("patch_fullscreen_menuitem", m_menu_fullscreen);
	xml->get_widget("patch_clear_menuitem", m_menu_clear);
	xml->get_widget("patch_destroy_menuitem", m_menu_destroy_patch);
	xml->get_widget("patch_add_plugin_menuitem", m_menu_add_plugin);
	xml->get_widget("patch_add_new_subpatch_menuitem", m_menu_new_subpatch);
	xml->get_widget("patch_add_subpatch_from_file_menuitem", m_menu_load_subpatch);
	xml->get_widget("patch_view_messages_window_menuitem", m_menu_view_messages_window);
	xml->get_widget("patch_view_patch_tree_window_menuitem", m_menu_view_patch_tree_window);
	xml->get_widget("patch_help_about_menuitem", m_menu_help_about);
	
	xml->get_widget_derived("load_plugin_win", m_load_plugin_window);
	xml->get_widget_derived("new_subpatch_win", m_new_subpatch_window);
	xml->get_widget_derived("load_patch_win", m_load_patch_window);
	xml->get_widget_derived("load_subpatch_win", m_load_subpatch_window);
	xml->get_widget_derived("patch_description_win", m_description_window);

	//m_load_plugin_window->set_transient_for(*this);
	m_new_subpatch_window->set_transient_for(*this);
	m_load_patch_window->set_transient_for(*this);
	m_load_subpatch_window->set_transient_for(*this);
	m_description_window->set_transient_for(*this);
	
	m_menu_view_control_window->property_sensitive() = false;
	//m_status_bar->push(Controller::instance().engine_url());
	//m_status_bar->pack_start(*Gtk::manage(new Gtk::Image(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_MENU)), false, false);
	
	m_menu_open->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_open));
	m_menu_open_into->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_open_into));
	m_menu_save->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save));
	m_menu_save_as->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save_as));
	m_menu_close->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_close));
	m_menu_quit->signal_activate().connect(
		sigc::mem_fun(App::instance(), &App::quit));
	m_menu_quit_and_kill->signal_activate().connect(
		sigc::mem_fun(App::instance(), &App::quit_and_kill));
	m_menu_configuration->signal_activate().connect(
		sigc::mem_fun(App::instance().configuration_dialog(), &ConfigWindow::show));
	m_menu_fullscreen->signal_toggled().connect(
		sigc::mem_fun(this, &PatchWindow::event_fullscreen_toggled));
	m_menu_view_control_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::show_control_window));
	m_menu_view_patch_description->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::show_description_window));
	m_menu_destroy_patch->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_destroy));
	m_menu_clear->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_clear));
	m_menu_add_plugin->signal_activate().connect(
		sigc::mem_fun<void>(m_load_plugin_window, &LoadPluginWindow::present));
	m_menu_new_subpatch->signal_activate().connect(
		sigc::mem_fun<void>(m_new_subpatch_window, &NewSubpatchWindow::present));
	m_menu_load_subpatch->signal_activate().connect(
		sigc::mem_fun<void>(m_load_subpatch_window, &LoadSubpatchWindow::present));
	m_menu_view_messages_window->signal_activate().connect(
		sigc::mem_fun<void>(App::instance().messages_dialog(), &MessagesWindow::present));
	m_menu_view_patch_tree_window->signal_activate().connect(
		sigc::mem_fun<void>(App::instance().patch_tree(), &PatchTreeWindow::present));

	// Temporary workaround for Gtkmm 2.4 (no AboutDialog)
	if (App::instance().about_dialog() != NULL) 
		m_menu_help_about->signal_activate().connect(
			sigc::mem_fun<void>(App::instance().about_dialog(), &Gtk::Dialog::present));

	App::instance().add_patch_window(this);
}


PatchWindow::~PatchWindow()
{
	App::instance().remove_patch_window(this);

	hide();
	
	delete m_new_subpatch_window;
	delete m_load_subpatch_window;
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
PatchWindow::patch_controller(PatchController* pc)
{
	m_enable_signal = false;

	assert(pc != NULL);
	assert(m_patch != pc);
	assert(m_patch == NULL ||
			pc->model()->path() != m_patch->model()->path());

	PatchController* old_pc = m_patch;
	if (old_pc != NULL) {
		assert(old_pc->window() == NULL || old_pc->window() == this);
		old_pc->claim_patch_view();
		old_pc->window(NULL);
	}

	m_patch = pc;

	if (pc->view() == NULL)
		pc->create_view();
	assert(pc->view() != NULL);

	PatchView* const patch_view = pc->view();
	assert(patch_view != NULL);
	patch_view->reparent(*m_viewport);
	pc->window(this);
	show_all();

	assert(m_load_plugin_window != NULL);
	assert(m_new_subpatch_window != NULL);
	assert(m_load_patch_window != NULL);
	assert(m_load_subpatch_window != NULL);

	m_load_patch_window->patch_controller(m_patch);
	m_load_plugin_window->patch_controller(m_patch);
	m_new_subpatch_window->patch_controller(m_patch);
	m_load_subpatch_window->patch_controller(m_patch);
	
	m_menu_view_control_window->property_sensitive() = pc->has_control_inputs();

	int width, height;
	get_size(width, height);
	patch_view->canvas()->scroll_to(
			((int)patch_view->canvas()->width() - width)/2,
			((int)patch_view->canvas()->height() - height)/2);

	set_title(m_patch->model()->path());

	m_description_window->patch_model(pc->patch_model());


	// Setup breadcrumbs box
	// FIXME: this is filthy

	// Moving to a parent patch, depress correct button
	if (old_pc != NULL &&
			old_pc->model()->path().substr(0, pc->model()->path().length())
			== pc->model()->path()) {
		for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i) {
			if ((*i)->patch() == pc)
				(*i)->set_active(true);
			else if ((*i)->patch() == old_pc)
				(*i)->set_active(false);
		}
	
	// Rebuild breadcrumbs from scratch (yeah, laziness..)
	} else {
		rebuild_breadcrumbs();
	}
	
	if (pc->model()->path() == "/")
		m_menu_destroy_patch->set_sensitive(false);
	else
		m_menu_destroy_patch->set_sensitive(true);
	
	assert(old_pc == NULL || old_pc->window() != this);
	assert(m_patch == pc);
	assert(m_patch->window() == this);

	m_enable_signal = true;
}


/** Destroys current breadcrumbs and rebuilds from scratch.
 * 
 * (Needs to be called when a patch is cleared to eliminate children crumbs)
 */
void
PatchWindow::rebuild_breadcrumbs()
{
		// Empty existing breadcrumbs
		for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i)
			m_breadcrumb_box->remove(**i);
		m_breadcrumbs.clear();

		// Add new ones
		string path = m_patch->path(); // To be chopped up, starting at the left
		string but_name;               // Name on breadcrumb button
		string but_path;               // Full path breadcrumb represents

		// Add root
		assert(path[0] == '/');
		BreadCrumb* but = manage(new BreadCrumb(this, Store::instance().patch("/")));
		m_breadcrumb_box->pack_start(*but, false, false, 1);
		m_breadcrumbs.push_back(but);
		path = path.substr(1); // hack off leading slash

		// Add the rest
		while (path.length() > 0) {
			if (path.find("/") != string::npos) {
				but_name = path.substr(0, path.find("/"));
				but_path += string("/") + path.substr(0, path.find("/"));
				path = path.substr(path.find("/")+1);
			} else {
				but_name = path;
				but_path += string("/") + path;
				path = "";
			}
			BreadCrumb* but = manage(new BreadCrumb(this, Store::instance().patch(but_path)));
			m_breadcrumb_box->pack_start(*but, false, false, 1);
			m_breadcrumbs.push_back(but);
		}
		(*m_breadcrumbs.back()).set_active(true);

}


void
PatchWindow::breadcrumb_clicked(BreadCrumb* crumb)
{
	if (m_enable_signal) {
		PatchController* const pc = crumb->patch();
		assert(pc != NULL);
	
		if (pc == m_patch) {
			crumb->set_active(true);
		} else if (pc->window() != NULL && pc->window()->is_visible()) {
			pc->show_patch_window();
			crumb->set_active(false);
		} else {
			patch_controller(pc);
		}
	}
}


void
PatchWindow::show_control_window()
{
	if (m_patch != NULL)
		m_patch->show_control_window();
}


void
PatchWindow::show_description_window()
{
	m_description_window->show();
}


/** Notification a node has been removed from the PatchView this window
 * currently contains.
 *
 * This is used to update the breadcrumbs in case the Node is a patch which has
 * a button present in the breadcrumbs that needs to be removed.
 */
void
PatchWindow::node_removed(const string& name)
{
	for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i) {
		if ((*i)->patch()->path() == m_patch->model()->base_path() + name) {
			for (list<BreadCrumb*>::iterator j = i; j != m_breadcrumbs.end(); ) {
				BreadCrumb* bc = *j;
				j = m_breadcrumbs.erase(j);
				m_breadcrumb_box->remove(*bc);
			}
			break;
		}
	}
}


/** Same as @a node_removed, but for renaming.
 */
void
PatchWindow::node_renamed(const string& old_path, const string& new_path)
{
	for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i) {
		if ((*i)->patch()->model()->path() == old_path)
			(*i)->path(new_path);
	}
}


/** Notification the patch this window is currently showing was renamed.
 */
void
PatchWindow::patch_renamed(const string& new_path)
{
	set_title(new_path);
	for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i) {
		if ((*i)->patch() == m_patch)
			(*i)->path(new_path);
	}
}


void
PatchWindow::event_open()
{
	m_load_patch_window->set_replace();
	m_load_patch_window->present();
}

void
PatchWindow::event_open_into()
{
	m_load_patch_window->set_merge();
	m_load_patch_window->present();
}


void
PatchWindow::event_save()
{
	PatchModel* const model = m_patch->patch_model();
	
	if (model->filename() == "")
		event_save_as();
	else
		Controller::instance().save_patch(model, model->filename(), false);
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
	const string& current_filename = m_patch->patch_model()->filename();
	if (current_filename.length() > 0)
		dialog.set_filename(current_filename);
	else if (App::instance().configuration()->patch_folder().length() > 0)
		dialog.set_current_folder(App::instance().configuration()->patch_folder());
	
	int result = dialog.run();
	bool recursive = recursive_checkbutton.get_active();
	
	assert(result == Gtk::RESPONSE_OK || result == Gtk::RESPONSE_CANCEL || result == Gtk::RESPONSE_NONE);
	
	if (result == Gtk::RESPONSE_OK) {	
		string filename = dialog.get_filename();
		if (filename.length() < 4 || filename.substr(filename.length()-3) != ".om")
			filename += ".om";
			
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
			Controller::instance().save_patch(m_patch->patch_model(), filename, recursive);
			m_patch->patch_model()->filename(filename);
		}
	}
	App::instance().configuration()->set_patch_folder(dialog.get_current_folder());
}


void
PatchWindow::on_show()
{
	if (m_position_stored)
		move(m_x, m_y);

	Gtk::Window::on_show();
}


void
PatchWindow::on_hide()
{
	m_position_stored = true;
	get_position(m_x, m_y);
	Gtk::Window::on_hide();
}


bool
PatchWindow::on_delete_event(GdkEventAny* ev)
{
	event_close();
	return true; // destroy window
}


bool
PatchWindow::on_key_press_event(GdkEventKey* event)
{
	if (event->keyval == GDK_Delete) {
		if (m_patch != NULL && m_patch->view() != NULL) {
			assert(m_patch->view()->canvas() != NULL);
			m_patch->view()->canvas()->destroy_selected();
		}
		return true;
	} else {
		return Gtk::Window::on_key_press_event(event);
	}
}


void
PatchWindow::event_close()
{
	if (App::instance().num_open_patch_windows() > 1) {
		hide();
	} else {
		Gtk::MessageDialog d(*this, "This is the last remaining open Om patch\
window.  Closing this window will exit OmGtk (the engine will remain running).\n\n\
Are you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
		int ret = d.run();
		if (ret == Gtk::RESPONSE_OK)
			App::instance().quit();
	}
}


void
PatchWindow::event_destroy()
{
	Controller::instance().destroy(m_patch->model()->path());
}


void
PatchWindow::event_clear()
{
	Controller::instance().clear_patch(m_patch->model()->path());
}

void
PatchWindow::event_fullscreen_toggled()
{
	if (m_menu_fullscreen->get_active())
		fullscreen();
	else
		unfullscreen();
}


} // namespace OmGtk
