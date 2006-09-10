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

#include "PatchWindow.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include "App.h"
#include "ModelEngineInterface.h"
#include "PatchView.h"
#include "OmFlowCanvas.h"
#include "PatchController.h"
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
#include "Loader.h"

namespace Ingenuity {


PatchWindow::PatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
: Gtk::Window(cobject),
  m_patch(NULL),
  m_load_plugin_window(NULL),
  m_new_subpatch_window(NULL),
  m_enable_signal(true),
  m_position_stored(false),
  m_x(0),
  m_y(0),
  m_breadcrumb_box(NULL)
{
	property_visible() = false;

	xml->get_widget("patch_win_vbox", m_vbox);
	xml->get_widget("patch_win_viewport", m_viewport);
	//xml->get_widget("patch_win_status_bar", m_status_bar);
	//xml->get_widget("patch_open_menuitem", m_menu_open);
	xml->get_widget("patch_import_menuitem", m_menu_import);
	//xml->get_widget("patch_open_into_menuitem", m_menu_open_into);
	xml->get_widget("patch_save_menuitem", m_menu_save);
	xml->get_widget("patch_save_as_menuitem", m_menu_save_as);
	xml->get_widget("patch_close_menuitem", m_menu_close);
	xml->get_widget("patch_configuration_menuitem", m_menu_configuration);
	xml->get_widget("patch_quit_menuitem", m_menu_quit);
	xml->get_widget("patch_view_control_window_menuitem", m_menu_view_control_window);
	xml->get_widget("patch_view_engine_window_menuitem", m_menu_view_engine_window);
	xml->get_widget("patch_properties_menuitem", m_menu_view_patch_properties);
	xml->get_widget("patch_fullscreen_menuitem", m_menu_fullscreen);
	xml->get_widget("patch_clear_menuitem", m_menu_clear);
	xml->get_widget("patch_destroy_menuitem", m_menu_destroy_patch);
	xml->get_widget("patch_view_messages_window_menuitem", m_menu_view_messages_window);
	xml->get_widget("patch_view_patch_tree_window_menuitem", m_menu_view_patch_tree_window);
	xml->get_widget("patch_help_about_menuitem", m_menu_help_about);
	
	xml->get_widget_derived("load_plugin_win", m_load_plugin_window);
	xml->get_widget_derived("new_subpatch_win", m_new_subpatch_window);
	xml->get_widget_derived("load_patch_win", m_load_patch_window);
	xml->get_widget_derived("load_subpatch_win", m_load_subpatch_window);

	m_new_subpatch_window->set_transient_for(*this);
	m_load_patch_window->set_transient_for(*this);
	m_load_subpatch_window->set_transient_for(*this);
	
	m_menu_view_control_window->property_sensitive() = false;
	//m_status_bar->push(App::instance().engine()->engine_url());
	//m_status_bar->pack_start(*Gtk::manage(new Gtk::Image(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_MENU)), false, false);
	
	/*m_menu_open->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_open));*/
	m_menu_import->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_import));
	m_menu_save->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save));
	m_menu_save_as->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save_as));
	m_menu_close->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_close));
	m_menu_quit->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_quit));
	m_menu_configuration->signal_activate().connect(
		sigc::mem_fun(App::instance().configuration_dialog(), &ConfigWindow::show));
	m_menu_fullscreen->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_fullscreen_toggled));
	m_menu_view_engine_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_engine));
	m_menu_view_control_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_controls));
	m_menu_view_patch_properties->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_properties));
	m_menu_destroy_patch->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_destroy));
	m_menu_clear->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_clear));
	m_menu_view_messages_window->signal_activate().connect(
		sigc::mem_fun<void>(App::instance().messages_dialog(), &MessagesWindow::present));
	m_menu_view_patch_tree_window->signal_activate().connect(
		sigc::mem_fun<void>(App::instance().patch_tree(), &PatchTreeWindow::present));

	// Temporary workaround for Gtkmm 2.4 (no AboutDialog)
	if (App::instance().about_dialog() != NULL) 
		m_menu_help_about->signal_activate().connect(
			sigc::mem_fun<void>(App::instance().about_dialog(), &Gtk::Dialog::present));
	
	m_breadcrumb_box = new BreadCrumbBox();
	m_breadcrumb_box->signal_patch_selected.connect(sigc::mem_fun(this, &PatchWindow::patch));

	App::instance().add_patch_window(this);
}


PatchWindow::~PatchWindow()
{
	App::instance().remove_patch_window(this);

	hide();
	
	delete m_new_subpatch_window;
	delete m_load_subpatch_window;
}


/** Set the patch controller from a Path (for BreadCrumbs)
 */
void 
PatchWindow::patch(const Path& path)
{	
	CountedPtr<PatchModel> model = App::instance().store()->object(path);
	if (!model)
		return; // can't really do anything useful..
	
	PatchController* pc = dynamic_cast<PatchController*>(model->controller());
	
	if (!pc) {
		pc = new PatchController(model);
		model->set_controller(pc);
	}

	assert(pc);

	if (pc->window() != NULL && pc->window()->is_visible()) {
		pc->show_patch_window();
	} else {
		patch_controller(pc);
	}
}


/** Sets the patch controller for this window and initializes everything.
 *
 * This function MUST be called before using the window in any way!
 */
void
PatchWindow::patch_controller(PatchController* pc)
{
	if (!pc || pc == m_patch)
		return;

	m_enable_signal = false;

	assert(pc);
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
	assert(pc->view());

	PatchView* const patch_view = pc->view();
	assert(patch_view != NULL);
	patch_view->reparent(*m_viewport);

	if (m_breadcrumb_box->get_parent())
		m_breadcrumb_box->reparent(*patch_view->breadcrumb_container());
	else
		patch_view->breadcrumb_container()->add(*m_breadcrumb_box);

	m_breadcrumb_box->build(pc->model()->path());
	m_breadcrumb_box->show();
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

	//m_properties_window->patch_model(pc->patch_model());

	if (pc->model()->path() == "/")
		m_menu_destroy_patch->set_sensitive(false);
	else
		m_menu_destroy_patch->set_sensitive(true);
	
	assert(old_pc == NULL || old_pc->window() != this);
	assert(m_patch == pc);
	assert(m_patch->window() == this);

	m_enable_signal = true;
}



void
PatchWindow::event_show_engine()
{
	if (m_patch)
		App::instance().connect_window()->show();
}


void
PatchWindow::event_show_controls()
{
	if (m_patch)
		m_patch->show_control_window();
}


void
PatchWindow::event_show_properties()
{
	if (m_patch)
		m_patch->show_properties_window();
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
	throw; // FIXME
/*
	for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i) {
		if ((*i)->path() == m_patch->model()->base_path() + name) {
			for (list<BreadCrumb*>::iterator j = i; j != m_breadcrumbs.end(); ) {
				BreadCrumb* bc = *j;
				j = m_breadcrumbs.erase(j);
				m_breadcrumb_box->remove(*bc);
			}
			break;
		}
	}*/
}


/** Same as @a node_removed, but for renaming.
 */
void
PatchWindow::node_renamed(const string& old_path, const string& new_path)
{
	throw; // FIXME
	/*
	for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i) {
		if ((*i)->path() == old_path)
			(*i)->set_path(new_path);
	}*/
}


/** Notification the patch this window is currently showing was renamed.
 */
void
PatchWindow::patch_renamed(const string& new_path)
{
	throw; // FIXME
	/*
	set_title(new_path);
	for (list<BreadCrumb*>::iterator i = m_breadcrumbs.begin(); i != m_breadcrumbs.end(); ++i) {
		if ((*i)->path() == m_patch->path())
			(*i)->set_path(new_path);
	}*/
}

/*
void
PatchWindow::event_open()
{
	m_load_patch_window->set_replace();
	m_load_patch_window->present();
}
*/

void
PatchWindow::event_import()
{
	m_load_patch_window->set_merge();
	m_load_patch_window->present();
}


void
PatchWindow::event_save()
{
	PatchModel* const model = m_patch->patch_model().get();
	
	if (model->filename() == "")
		event_save_as();
	else
		App::instance().loader()->save_patch(model, model->filename(), false);
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
			App::instance().loader()->save_patch(m_patch->patch_model().get(), filename, recursive);
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
	claim_breadcrumbs();
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
		Gtk::MessageDialog d(*this, "This is the last remaining open patch "
			"window.  Closing this window will exit Ingenuity (the engine will "
			"remain running).\n\nAre you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
			d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		int ret = d.run();
		if (ret == Gtk::RESPONSE_CLOSE)
			App::instance().quit();
		else
			d.hide();
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
	App::instance().engine()->destroy(m_patch->model()->path());
}


void
PatchWindow::event_clear()
{
	App::instance().engine()->clear_patch(m_patch->model()->path());
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

void
PatchWindow::claim_breadcrumbs()
{
	m_breadcrumb_box->reparent(m_breadcrumb_bin);
}


} // namespace Ingenuity
