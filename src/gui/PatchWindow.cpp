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

#include <cassert>
#include <sstream>

#include <boost/format.hpp>
#include <glib/gstdio.h>
#include <glibmm/fileutils.h>

#include "raul/AtomRDF.hpp"

#include "ingen/ServerInterface.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/shared/LV2URIMap.hpp"

#include "App.hpp"
#include "BreadCrumbs.hpp"
#include "Configuration.hpp"
#include "ConnectWindow.hpp"
#include "LoadPatchWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "MessagesWindow.hpp"
#include "NewSubpatchWindow.hpp"
#include "NodeControlWindow.hpp"
#include "PatchCanvas.hpp"
#include "PatchTreeWindow.hpp"
#include "PatchView.hpp"
#include "PatchWindow.hpp"
#include "ThreadedLoader.hpp"
#include "WindowFactory.hpp"
#include "ingen-config.h"

#ifdef HAVE_WEBKIT
#include <webkit/webkit.h>
#endif

using namespace Raul;

namespace Ingen {
namespace GUI {

static const int STATUS_CONTEXT_ENGINE = 0;
static const int STATUS_CONTEXT_PATCH  = 1;
static const int STATUS_CONTEXT_HOVER  = 2;

PatchWindow::PatchWindow(BaseObjectType*                   cobject,
                         const Glib::RefPtr<Gtk::Builder>& xml)
	: Window(cobject)
	, _enable_signal(true)
	, _position_stored(false)
	, _x(0)
	, _y(0)
	, _breadcrumbs(NULL)
	, _has_shown_documentation(false)
{
	property_visible() = false;

	xml->get_widget("patch_win_vbox", _vbox);
	xml->get_widget("patch_win_alignment", _alignment);
	xml->get_widget("patch_win_status_bar", _status_bar);
	//xml->get_widget("patch_win_status_bar", _status_bar);
	//xml->get_widget("patch_open_menuitem", _menu_open);
	xml->get_widget("patch_import_menuitem", _menu_import);
	//xml->get_widget("patch_open_into_menuitem", _menu_open_into);
	xml->get_widget("patch_save_menuitem", _menu_save);
	xml->get_widget("patch_save_as_menuitem", _menu_save_as);
	xml->get_widget("patch_draw_menuitem", _menu_draw);
	xml->get_widget("patch_edit_controls_menuitem", _menu_edit_controls);
	xml->get_widget("patch_cut_menuitem", _menu_cut);
	xml->get_widget("patch_copy_menuitem", _menu_copy);
	xml->get_widget("patch_paste_menuitem", _menu_paste);
	xml->get_widget("patch_delete_menuitem", _menu_delete);
	xml->get_widget("patch_select_all_menuitem", _menu_select_all);
	xml->get_widget("patch_close_menuitem", _menu_close);
	xml->get_widget("patch_quit_menuitem", _menu_quit);
	xml->get_widget("patch_view_control_window_menuitem", _menu_view_control_window);
	xml->get_widget("patch_view_engine_window_menuitem", _menu_view_engine_window);
	xml->get_widget("patch_properties_menuitem", _menu_view_patch_properties);
	xml->get_widget("patch_fullscreen_menuitem", _menu_fullscreen);
	xml->get_widget("patch_human_names_menuitem", _menu_human_names);
	xml->get_widget("patch_show_port_names_menuitem", _menu_show_port_names);
	xml->get_widget("patch_zoom_in_menuitem", _menu_zoom_in);
	xml->get_widget("patch_zoom_out_menuitem", _menu_zoom_out);
	xml->get_widget("patch_zoom_normal_menuitem", _menu_zoom_normal);
	xml->get_widget("patch_status_bar_menuitem", _menu_show_status_bar);
	xml->get_widget("patch_arrange_menuitem", _menu_arrange);
	xml->get_widget("patch_view_messages_window_menuitem", _menu_view_messages_window);
	xml->get_widget("patch_view_patch_tree_window_menuitem", _menu_view_patch_tree_window);
	xml->get_widget("patch_help_about_menuitem", _menu_help_about);
	xml->get_widget("patch_documentation_paned", _doc_paned);
	xml->get_widget("patch_documentation_scrolledwindow", _doc_scrolledwindow);

	_menu_view_control_window->property_sensitive() = false;
	_menu_import->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_import));
	_menu_save->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save));
	_menu_save_as->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_save_as));
	_menu_draw->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_draw));
	_menu_edit_controls->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_edit_controls));
	_menu_copy->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_copy));
	_menu_paste->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_paste));
	_menu_delete->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_delete));
	_menu_select_all->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_select_all));
	_menu_close->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_close));
	_menu_quit->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_quit));
	_menu_fullscreen->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_fullscreen_toggled));
	_menu_human_names->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_human_names_toggled));
	_menu_show_status_bar->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_status_bar_toggled));
	_menu_show_port_names->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_port_names_toggled));
	_menu_arrange->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_arrange));
	_menu_quit->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_quit));
	_menu_zoom_in->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_zoom_in));
	_menu_zoom_out->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_zoom_out));
	_menu_zoom_normal->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_zoom_normal));
	_menu_view_engine_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_engine));
	_menu_view_control_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_controls));
	_menu_view_patch_properties->signal_activate().connect(
		sigc::mem_fun(this, &PatchWindow::event_show_properties));

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->signal_owner_change().connect(
		sigc::mem_fun(this, &PatchWindow::event_clipboard_changed));
}

PatchWindow::~PatchWindow()
{
	delete _breadcrumbs;
}

void
PatchWindow::init_window(App& app)
{
	Window::init_window(app);

	string engine_name = _app->engine()->uri().str();
	if (engine_name == "http://drobilla.net/ns/ingen#internal") {
		engine_name = "internal engine";
	}
	_status_bar->push(string("Connected to ") + engine_name, STATUS_CONTEXT_ENGINE);

	_menu_view_messages_window->signal_activate().connect(
		sigc::mem_fun<void>(_app->messages_dialog(), &MessagesWindow::present));
	_menu_view_patch_tree_window->signal_activate().connect(
		sigc::mem_fun<void>(_app->patch_tree(), &PatchTreeWindow::present));

	_menu_help_about->signal_activate().connect(sigc::hide_return(
		sigc::mem_fun(_app, &App::show_about)));

	_breadcrumbs = new BreadCrumbs(*_app);
	_breadcrumbs->signal_patch_selected.connect(
		sigc::mem_fun(this, &PatchWindow::set_patch_from_path));
}

/** Set the patch controller from a Path (for use by eg. BreadCrumbs)
 */
void
PatchWindow::set_patch_from_path(const Path& path, SharedPtr<PatchView> view)
{
	if (view) {
		assert(view->patch()->path() == path);
		_app->window_factory()->present_patch(view->patch(), this, view);
	} else {
		SharedPtr<const PatchModel> model = PtrCast<const PatchModel>(
			_app->store()->object(path));
		if (model)
			_app->window_factory()->present_patch(model, this);
	}
}

/** Sets the patch controller for this window and initializes everything.
 *
 * If @a view is NULL, a new view will be created.
 */
void
PatchWindow::set_patch(SharedPtr<const PatchModel> patch,
                       SharedPtr<PatchView>        view)
{
	if (!patch || patch == _patch)
		return;

	_enable_signal = false;

	new_port_connection.disconnect();
	removed_port_connection.disconnect();
	edit_mode_connection.disconnect();
	_entered_connection.disconnect();
	_left_connection.disconnect();

	_status_bar->pop(STATUS_CONTEXT_PATCH);

	_patch = patch;
	_view  = view;

	if (!_view)
		_view = _breadcrumbs->view(patch->path());

	if (!_view)
		_view = PatchView::create(*_app, patch);

	assert(_view);

	// Add view to our alignment
	if (_view->get_parent())
		_view->get_parent()->remove(*_view.get());

	_alignment->remove();
	_alignment->add(*_view.get());

	if (_breadcrumbs->get_parent())
		_breadcrumbs->get_parent()->remove(*_breadcrumbs);

	_view->breadcrumb_container()->remove();
	_view->breadcrumb_container()->add(*_breadcrumbs);
	_view->breadcrumb_container()->show();

	_breadcrumbs->build(patch->path(), _view);
	_breadcrumbs->show();

	_menu_view_control_window->property_sensitive() = false;

	for (NodeModel::Ports::const_iterator p = patch->ports().begin();
			p != patch->ports().end(); ++p) {
		if (_app->can_control(p->get())) {
			_menu_view_control_window->property_sensitive() = true;
			break;
		}
	}

	set_title(_patch->path().chop_scheme() + " - Ingen");

	new_port_connection = patch->signal_new_port().connect(
		sigc::mem_fun(this, &PatchWindow::patch_port_added));
	removed_port_connection = patch->signal_removed_port().connect(
		sigc::mem_fun(this, &PatchWindow::patch_port_removed));
	removed_port_connection = patch->signal_editable().connect(
		sigc::mem_fun(this, &PatchWindow::editable_changed));

	show();
	_alignment->show_all();

	_view->signal_object_entered.connect(
		sigc::mem_fun(this, &PatchWindow::object_entered));
	_view->signal_object_left.connect(
		sigc::mem_fun(this, &PatchWindow::object_left));

	_enable_signal = true;
}

void
PatchWindow::patch_port_added(SharedPtr<const PortModel> port)
{
	if (port->is_input() && _app->can_control(port.get())) {
		_menu_view_control_window->property_sensitive() = true;
	}
}

void
PatchWindow::patch_port_removed(SharedPtr<const PortModel> port)
{
	if (!(port->is_input() && _app->can_control(port.get())))
		return;

	for (NodeModel::Ports::const_iterator i = _patch->ports().begin();
			i != _patch->ports().end(); ++i) {
		if ((*i)->is_input() && _app->can_control(i->get())) {
			_menu_view_control_window->property_sensitive() = true;
			return;
		}
	}

	_menu_view_control_window->property_sensitive() = false;
}

void
PatchWindow::show_documentation(const std::string& doc, bool html)
{
#ifdef HAVE_WEBKIT
	WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	webkit_web_view_load_html_string(view, doc.c_str(), "");
	_doc_scrolledwindow->add(*Gtk::manage(Glib::wrap(GTK_WIDGET(view))));
	_doc_scrolledwindow->show_all();
#else
	Gtk::TextView* view = Gtk::manage(new Gtk::TextView());
	view->get_buffer()->set_text(doc);
	_doc_scrolledwindow->add(*view);
	_doc_scrolledwindow->show_all();
#endif
	if (!_has_shown_documentation) {
		int width, height;
		get_size(width, height);
		_doc_paned->set_position(width / 1.61803399);
	}
	_has_shown_documentation = true;
}

void
PatchWindow::hide_documentation()
{
	_doc_scrolledwindow->remove();
	_doc_scrolledwindow->hide();
}
	
void
PatchWindow::show_status(const ObjectModel* model)
{
	std::stringstream msg;
	msg << model->path().chop_scheme();

	const PortModel* port = 0;
	const NodeModel* node = 0;

	if ((port = dynamic_cast<const PortModel*>(model))) {
		show_port_status(port, port->value());

	} else if ((node = dynamic_cast<const NodeModel*>(model))) {
		const PluginModel* plugin = dynamic_cast<const PluginModel*>(node->plugin());
		if (plugin)
			msg << ((boost::format(" (%1%)") % plugin->human_name()).str());
		_status_bar->push(msg.str(), STATUS_CONTEXT_HOVER);
	}
}

void
PatchWindow::show_port_status(const PortModel* port, const Raul::Atom& value)
{
	std::stringstream msg;
	msg << port->path().chop_scheme();

	const NodeModel* parent = dynamic_cast<const NodeModel*>(port->parent().get());
	if (parent) {
		const PluginModel* plugin = dynamic_cast<const PluginModel*>(parent->plugin());
		if (plugin) {
			const string& human_name = plugin->port_human_name(port->index());
			if (!human_name.empty())
				msg << " (" << human_name << ")";
		}
	}

	if (value.is_valid()) {
		msg << " = " << value;
	}

	_status_bar->pop(STATUS_CONTEXT_HOVER);
	_status_bar->push(msg.str(), STATUS_CONTEXT_HOVER);
}

void
PatchWindow::object_entered(const ObjectModel* model)
{
	show_status(model);
}

void
PatchWindow::object_left(const ObjectModel* model)
{
	_status_bar->pop(STATUS_CONTEXT_PATCH);
	_status_bar->pop(STATUS_CONTEXT_HOVER);
}

void
PatchWindow::editable_changed(bool editable)
{
	_menu_edit_controls->set_active(editable);
}

void
PatchWindow::event_show_engine()
{
	if (_patch)
		_app->connect_window()->show();
}

void
PatchWindow::event_clipboard_changed(GdkEventOwnerChange* ev)
{
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	_menu_paste->set_sensitive(clipboard->wait_is_text_available());
}

void
PatchWindow::event_show_controls()
{
	_app->window_factory()->present_controls(_patch);
}

void
PatchWindow::event_show_properties()
{
	_app->window_factory()->present_properties(_patch);
}

void
PatchWindow::event_import()
{
	_app->window_factory()->present_load_patch(_patch);
}

void
PatchWindow::event_save()
{
	const Raul::Atom& document = _patch->get_property(_app->uris().ingen_document);
	if (!document.is_valid() || document.type() != Raul::Atom::URI) {
		event_save_as();
	} else {
		_app->loader()->save_patch(_patch, document.get_uri());
		_status_bar->push(
				(boost::format("Saved %1% to %2%") % _patch->path().chop_scheme()
				 % document.get_uri()).str(),
				STATUS_CONTEXT_PATCH);
	}
}

void
PatchWindow::event_save_as()
{
	const Shared::URIs& uris = _app->uris();
	while (true) {
		Gtk::FileChooserDialog dialog(*this, "Save Patch", Gtk::FILE_CHOOSER_ACTION_SAVE);

		dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		Gtk::Button* save_button = dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
		save_button->property_has_default() = true;

		Gtk::FileFilter filt;
		filt.add_pattern("*.ingen");
		filt.set_name("Ingen bundles");
		dialog.set_filter(filt);

		// Set current folder to most sensible default
		const Raul::Atom& document = _patch->get_property(uris.ingen_document);
		if (document.type() == Raul::Atom::URI)
			dialog.set_uri(document.get_uri());
		else if (_app->configuration()->patch_folder().length() > 0)
			dialog.set_current_folder(_app->configuration()->patch_folder());

		if (dialog.run() != Gtk::RESPONSE_OK)
			break;

		std::string filename = dialog.get_filename();
		std::string basename = Glib::path_get_basename(filename);

		if (basename.find('.') == string::npos) {
			filename += ".ingen";
			basename += ".ingen";
		} else if (filename.substr(filename.length() - 10) != ".ingen") {
			Gtk::MessageDialog error_dialog(*this,
"<b>" "Ingen patches must be saved to Ingen bundles (*.ingen)." "</b>",
					true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			error_dialog.run();
			continue;
		}

		const std::string symbol(basename.substr(0, basename.find('.')));

		if (!Symbol::is_valid(symbol)) {
			Gtk::MessageDialog error_dialog(*this,
					"<b>" "Ingen bundle names must be valid symbols." "</b>",
					true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			error_dialog.set_secondary_text(
"All characters must be _, a-z, A-Z, or 0-9, but the first may not be 0-9.");
			error_dialog.run();
			continue;
		}

		//_patch->set_property(uris.lv2_symbol, Atom(symbol.c_str()));

		bool confirm = true;
		if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
			if (Glib::file_test(Glib::build_filename(filename, "manifest.ttl"),
						Glib::FILE_TEST_EXISTS)) {
				Gtk::MessageDialog confirm_dialog(*this, (boost::format("<b>"
						"A bundle named \"%1%\" already exists.  Replace it?"
						"</b>") % basename).str(),
						true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
				confirm = (confirm_dialog.run() == Gtk::RESPONSE_YES);
			} else {
				Gtk::MessageDialog confirm_dialog(*this, (boost::format("<b>"
"A directory named \"%1%\" already exists, but is not an Ingen bundle.  \
Save into it anyway?" "</b>") % basename).str(),
						true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
				confirm_dialog.set_secondary_text(
"This will create at least 2 .ttl files in this directory, and possibly several \
more files and/or directories, recursively.  Existing files will be overwritten.");
				confirm = (confirm_dialog.run() == Gtk::RESPONSE_YES);
			}
		} else if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
			Gtk::MessageDialog confirm_dialog(*this, (boost::format("<b>"
"A file named \"%1%\" already exists.  Replace it with an Ingen bundle?" "</b>") % basename).str(),
					true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			confirm = (confirm_dialog.run() == Gtk::RESPONSE_YES);
			if (confirm)
				::g_remove(filename.c_str());
		}

		if (confirm) {
			const Glib::ustring uri = Glib::filename_to_uri(filename);
			_app->loader()->save_patch(_patch, uri);
			const_cast<PatchModel*>(_patch.get())->set_property(
				uris.ingen_document, Atom(Atom::URI, uri.c_str()), Resource::EXTERNAL);
			_status_bar->push(
					(boost::format("Saved %1% to %2%") % _patch->path().chop_scheme()
					 % filename).str(),
					STATUS_CONTEXT_PATCH);
		}

		_app->configuration()->set_patch_folder(dialog.get_current_folder());
		break;
	}
}

void
PatchWindow::event_draw()
{
	Gtk::FileChooserDialog dialog(*this, "Draw to DOT", Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	Gtk::Button* save_button = dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	save_button->property_has_default() = true;

	int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		string filename = dialog.get_filename();
		if (filename.find(".") == string::npos)
			filename += ".dot";

		bool confirm = true;
		if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
			const string msg = string("File exists!\n")
				+ "Are you sure you want to overwrite " + filename + "?";
			Gtk::MessageDialog confirm_dialog(*this,
				msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			confirm = (confirm_dialog.run() == Gtk::RESPONSE_YES);
		}

		if (confirm) {
			_view->canvas()->render_to_dot(filename);
			_status_bar->push(
					(boost::format("Rendered %1% to %2%") % _patch->path() % filename).str(),
					STATUS_CONTEXT_PATCH);
		}
	}
}

void
PatchWindow::event_edit_controls()
{
	if (_view)
		_view->set_editable(_menu_edit_controls->get_active());
}

void
PatchWindow::event_copy()
{
	if (_view)
		_view->canvas()->copy_selection();
}

void
PatchWindow::event_paste()
{
	if (_view)
		_view->canvas()->paste();
}

void
PatchWindow::event_delete()
{
	if (_view)
		_view->canvas()->destroy_selection();
}

void
PatchWindow::event_select_all()
{
	if (_view)
		_view->canvas()->select_all();
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
PatchWindow::on_event(GdkEvent* event)
{
	if ((event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE)
			&& _view->canvas()->on_event(event)) {
		return true;
	} else {
		return Gtk::Window::on_event(event);
	}
}

void
PatchWindow::event_close()
{
	_app->window_factory()->remove_patch_window(this);
}

void
PatchWindow::event_quit()
{
	_app->quit(*this);
}

void
PatchWindow::event_zoom_in()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() + 1.0);
}

void
PatchWindow::event_zoom_out()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() - 1.0);
}

void
PatchWindow::event_zoom_normal()
{
	_view->canvas()->set_zoom_and_font_size(1.0, _view->canvas()->get_default_font_size());
}

void
PatchWindow::event_arrange()
{
	_view->canvas()->arrange(false);
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
PatchWindow::event_status_bar_toggled()
{
	if (_menu_show_status_bar->get_active())
		_status_bar->show();
	else
		_status_bar->hide();
}

void
PatchWindow::event_human_names_toggled()
{
	_view->canvas()->show_human_names(_menu_human_names->get_active());
	if (_menu_human_names->get_active())
		_app->configuration()->set_name_style(Configuration::HUMAN);
	else
		_app->configuration()->set_name_style(Configuration::PATH);
}

void
PatchWindow::event_port_names_toggled()
{
	if (_menu_show_port_names->get_active()) {
		_view->canvas()->set_direction(GANV_DIRECTION_RIGHT);
		_view->canvas()->show_port_names(true);
	} else {
		_view->canvas()->set_direction(GANV_DIRECTION_DOWN);
		_view->canvas()->show_port_names(false);
	}
}

} // namespace GUI
} // namespace Ingen
