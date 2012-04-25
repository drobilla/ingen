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

#include <cassert>
#include <sstream>

#include <boost/format.hpp>
#include <glib/gstdio.h>
#include <glibmm/fileutils.h>

#include "ingen/Interface.hpp"
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
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"
#include "ingen_config.h"

#ifdef HAVE_WEBKIT
#include <webkit/webkit.h>
#endif

using namespace Raul;

namespace Ingen {
namespace GUI {

static const int STATUS_CONTEXT_ENGINE = 0;
static const int STATUS_CONTEXT_PATCH  = 1;
static const int STATUS_CONTEXT_HOVER  = 2;

PatchBox::PatchBox(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::VBox(cobject)
	, _app(NULL)
	, _window(NULL)
	, _breadcrumbs(NULL)
	, _has_shown_documentation(false)
	, _enable_signal(true)
{
	property_visible() = false;

	xml->get_widget("patch_win_alignment", _alignment);
	xml->get_widget("patch_win_status_bar", _status_bar);
	//xml->get_widget("patch_win_status_bar", _status_bar);
	//xml->get_widget("patch_open_menuitem", _menu_open);
	xml->get_widget("patch_import_menuitem", _menu_import);
	//xml->get_widget("patch_open_into_menuitem", _menu_open_into);
	xml->get_widget("patch_save_menuitem", _menu_save);
	xml->get_widget("patch_save_as_menuitem", _menu_save_as);
	xml->get_widget("patch_draw_menuitem", _menu_draw);
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
		sigc::mem_fun(this, &PatchBox::event_import));
	_menu_save->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_save));
	_menu_save_as->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_save_as));
	_menu_draw->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_draw));
	_menu_copy->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_copy));
	_menu_paste->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_paste));
	_menu_delete->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_delete));
	_menu_select_all->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_select_all));
	_menu_close->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_close));
	_menu_quit->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_quit));
	_menu_fullscreen->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_fullscreen_toggled));
	_menu_human_names->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_human_names_toggled));
	_menu_show_status_bar->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_status_bar_toggled));
	_menu_show_port_names->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_port_names_toggled));
	_menu_arrange->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_arrange));
	_menu_quit->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_quit));
	_menu_zoom_in->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_zoom_in));
	_menu_zoom_out->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_zoom_out));
	_menu_zoom_normal->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_zoom_normal));
	_menu_view_engine_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_show_engine));
	_menu_view_control_window->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_show_controls));
	_menu_view_patch_properties->signal_activate().connect(
		sigc::mem_fun(this, &PatchBox::event_show_properties));

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->signal_owner_change().connect(
		sigc::mem_fun(this, &PatchBox::event_clipboard_changed));
}

PatchBox::~PatchBox()
{
	delete _breadcrumbs;
}

SharedPtr<PatchBox>
PatchBox::create(App& app, SharedPtr<const PatchModel> patch)
{
	PatchBox* result = NULL;
	Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create("patch_win");
	xml->get_widget_derived("patch_win_vbox", result);
	result->init_box(app);
	result->set_patch(patch, SharedPtr<PatchView>());
	return SharedPtr<PatchBox>(result);
}

void
PatchBox::init_box(App& app)
{
	_app = &app;

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
		sigc::mem_fun(this, &PatchBox::set_patch_from_path));
}

void
PatchBox::set_patch_from_path(const Raul::Path& path, SharedPtr<PatchView> view)
{
	if (view) {
		assert(view->patch()->path() == path);
		_app->window_factory()->present_patch(view->patch(), _window, view);
	} else {
		SharedPtr<const PatchModel> model = PtrCast<const PatchModel>(
			_app->store()->object(path));
		if (model) {
			_app->window_factory()->present_patch(model, _window);
		}
	}
}

/** Sets the patch for this box and initializes everything.
 *
 * If @a view is NULL, a new view will be created.
 */
void
PatchBox::set_patch(SharedPtr<const PatchModel> patch,
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

	new_port_connection = patch->signal_new_port().connect(
		sigc::mem_fun(this, &PatchBox::patch_port_added));
	removed_port_connection = patch->signal_removed_port().connect(
		sigc::mem_fun(this, &PatchBox::patch_port_removed));

	show();
	_alignment->show_all();
	hide_documentation();

	_view->signal_object_entered.connect(
		sigc::mem_fun(this, &PatchBox::object_entered));
	_view->signal_object_left.connect(
		sigc::mem_fun(this, &PatchBox::object_left));

	_enable_signal = true;
}

void
PatchBox::patch_port_added(SharedPtr<const PortModel> port)
{
	if (port->is_input() && _app->can_control(port.get())) {
		_menu_view_control_window->property_sensitive() = true;
	}
}

void
PatchBox::patch_port_removed(SharedPtr<const PortModel> port)
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
PatchBox::show_documentation(const std::string& doc, bool html)
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
		const Gtk::Allocation allocation = get_allocation();
		_doc_paned->set_position(allocation.get_width() / 1.61803399);
	}
	_has_shown_documentation = true;
}

void
PatchBox::hide_documentation()
{
	_doc_scrolledwindow->remove();
	_doc_scrolledwindow->hide();
	_doc_paned->set_position(INT_MAX);
}

void
PatchBox::show_status(const ObjectModel* model)
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
PatchBox::show_port_status(const PortModel* port, const Raul::Atom& value)
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
		msg << " = " << _app->forge().str(value);
	}

	_status_bar->pop(STATUS_CONTEXT_HOVER);
	_status_bar->push(msg.str(), STATUS_CONTEXT_HOVER);
}

void
PatchBox::object_entered(const ObjectModel* model)
{
	show_status(model);
}

void
PatchBox::object_left(const ObjectModel* model)
{
	_status_bar->pop(STATUS_CONTEXT_PATCH);
	_status_bar->pop(STATUS_CONTEXT_HOVER);
}

void
PatchBox::event_show_engine()
{
	if (_patch)
		_app->connect_window()->show();
}

void
PatchBox::event_clipboard_changed(GdkEventOwnerChange* ev)
{
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	_menu_paste->set_sensitive(clipboard->wait_is_text_available());
}

void
PatchBox::event_show_controls()
{
	_app->window_factory()->present_controls(_patch);
}

void
PatchBox::event_show_properties()
{
	_app->window_factory()->present_properties(_patch);
}

void
PatchBox::event_import()
{
	_app->window_factory()->present_load_patch(_patch);
}

void
PatchBox::event_save()
{
	const Raul::Atom& document = _patch->get_property(_app->uris().ingen_document);
	if (!document.is_valid() || document.type() != _app->uris().forge.URI) {
		event_save_as();
	} else {
		_app->loader()->save_patch(_patch, document.get_uri());
		_status_bar->push(
			(boost::format("Saved %1% to %2%") % _patch->path().chop_scheme()
			 % document.get_uri()).str(),
			STATUS_CONTEXT_PATCH);
	}
}

int
PatchBox::message_dialog(const Glib::ustring& message,
                         const Glib::ustring& secondary_text,
                         Gtk::MessageType     type,
                         Gtk::ButtonsType     buttons)
{
	Gtk::MessageDialog dialog(message, true, type, buttons, true);
	dialog.set_secondary_text(secondary_text);
	if (_window) {
		dialog.set_transient_for(*_window);
	}
	return dialog.run();
}

void
PatchBox::event_save_as()
{
	const Shared::URIs& uris = _app->uris();
	while (true) {
		Gtk::FileChooserDialog dialog("Save Patch", Gtk::FILE_CHOOSER_ACTION_SAVE);
		if (_window) {
			dialog.set_transient_for(*_window);
		}

		dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		Gtk::Button* save_button = dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
		save_button->property_has_default() = true;

		Gtk::FileFilter filt;
		filt.add_pattern("*.ingen");
		filt.set_name("Ingen bundles");
		dialog.set_filter(filt);

		// Set current folder to most sensible default
		const Raul::Atom& document = _patch->get_property(uris.ingen_document);
		if (document.type() == uris.forge.URI)
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
			message_dialog(
				"<b>Ingen patches must be saved to Ingen bundles (*.ingen).</b>",
				"", Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
			continue;
		}

		const std::string symbol(basename.substr(0, basename.find('.')));

		if (!Symbol::is_valid(symbol)) {
			message_dialog(
				"<b>Ingen bundle names must be valid symbols.</b>",
				"All characters must be _, a-z, A-Z, or 0-9, but the first may not be 0-9.",
				Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
			continue;
		}

		//_patch->set_property(uris.lv2_symbol, Atom(symbol.c_str()));

		bool confirm = true;
		if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
			if (Glib::file_test(Glib::build_filename(filename, "manifest.ttl"),
			                    Glib::FILE_TEST_EXISTS)) {
				int ret = message_dialog(
					(boost::format("<b>A bundle named \"%1%\" already exists."
					               "  Replace it?</b>") % basename).str(),
					"", Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
				confirm = (ret == Gtk::RESPONSE_YES);
			} else {
				int ret = message_dialog(
					(boost::format("<b>A directory named \"%1%\" already exists,"
					               "but is not an Ingen bundle.  "
					               "Save into it anyway?</b>") % basename).str(),
					"This will create at least 2 .ttl files in this directory,"
					"and possibly several more files and/or directories, recursively.  "
					"Existing files will be overwritten.",
					Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
				confirm = (ret == Gtk::RESPONSE_YES);
			}
		} else if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
			int ret = message_dialog(
				(boost::format("<b>A file named \"%1%\" already exists.  "
				               "Replace it with an Ingen bundle?</b>")
				 % basename).str(),
				"", Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
			confirm = (ret == Gtk::RESPONSE_YES);
			if (confirm)
				::g_remove(filename.c_str());
		}

		if (confirm) {
			const Glib::ustring uri = Glib::filename_to_uri(filename);
			_app->loader()->save_patch(_patch, uri);
			const_cast<PatchModel*>(_patch.get())->set_property(
				uris.ingen_document,
				_app->forge().alloc_uri(uri.c_str()),
				Resource::EXTERNAL);
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
PatchBox::event_draw()
{
	Gtk::FileChooserDialog dialog("Draw to DOT", Gtk::FILE_CHOOSER_ACTION_SAVE);
	if (_window) {
		dialog.set_transient_for(*_window);
	}

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
			int ret = message_dialog(
				(boost::format("File exists!  Overwrite %1%?") % filename).str(),
				"", Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
			confirm = (ret == Gtk::RESPONSE_YES);
		}

		if (confirm) {
			_view->canvas()->export_dot(filename.c_str());
			_status_bar->push(
				(boost::format("Rendered %1% to %2%") % _patch->path() % filename).str(),
				STATUS_CONTEXT_PATCH);
		}
	}
}

void
PatchBox::event_copy()
{
	if (_view)
		_view->canvas()->copy_selection();
}

void
PatchBox::event_paste()
{
	if (_view)
		_view->canvas()->paste();
}

void
PatchBox::event_delete()
{
	if (_view)
		_view->canvas()->destroy_selection();
}

void
PatchBox::event_select_all()
{
	if (_view)
		_view->canvas()->select_all();
}

void
PatchBox::event_close()
{
	if (_window) {
		_app->window_factory()->remove_patch_window(_window);
	}
}

void
PatchBox::event_quit()
{
	_app->quit(_window);
}

void
PatchBox::event_zoom_in()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() + 1.0);
}

void
PatchBox::event_zoom_out()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() - 1.0);
}

void
PatchBox::event_zoom_normal()
{
	_view->canvas()->set_scale(1.0, _view->canvas()->get_default_font_size());
}

void
PatchBox::event_arrange()
{
	_view->canvas()->arrange();
}

void
PatchBox::event_fullscreen_toggled()
{
	// FIXME: ugh, use GTK signals to track state and know for sure
	static bool is_fullscreen = false;

	if (_window) {
		if (!is_fullscreen) {
			_window->fullscreen();
			is_fullscreen = true;
		} else {
			_window->unfullscreen();
			is_fullscreen = false;
		}
	}
}

void
PatchBox::event_status_bar_toggled()
{
	if (_menu_show_status_bar->get_active())
		_status_bar->show();
	else
		_status_bar->hide();
}

void
PatchBox::event_human_names_toggled()
{
	_view->canvas()->show_human_names(_menu_human_names->get_active());
	if (_menu_human_names->get_active())
		_app->configuration()->set_name_style(Configuration::HUMAN);
	else
		_app->configuration()->set_name_style(Configuration::PATH);
}

void
PatchBox::event_port_names_toggled()
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
