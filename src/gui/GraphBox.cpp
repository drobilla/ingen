/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

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
#include <string>

#include <boost/format.hpp>
#include <glib/gstdio.h>
#include <glibmm/fileutils.h>
#include <gtkmm/stock.h>

#include "ingen/Configuration.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "BreadCrumbs.hpp"
#include "ConnectWindow.hpp"
#include "GraphCanvas.hpp"
#include "GraphTreeWindow.hpp"
#include "GraphView.hpp"
#include "GraphWindow.hpp"
#include "LoadGraphWindow.hpp"
#include "LoadPluginWindow.hpp"
#include "MessagesWindow.hpp"
#include "NewSubgraphWindow.hpp"
#include "Style.hpp"
#include "ThreadedLoader.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"
#include "ingen_config.h"

#ifdef HAVE_WEBKIT
#include <webkit/webkit.h>
#endif

namespace ingen {

using namespace client;

namespace gui {

static const int STATUS_CONTEXT_ENGINE = 0;
static const int STATUS_CONTEXT_GRAPH  = 1;
static const int STATUS_CONTEXT_HOVER  = 2;

GraphBox::GraphBox(BaseObjectType*                   cobject,
                   const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::VBox(cobject)
	, _app(nullptr)
	, _window(nullptr)
	, _breadcrumbs(nullptr)
	, _has_shown_documentation(false)
	, _enable_signal(true)
{
	property_visible() = false;

	xml->get_widget("graph_win_alignment", _alignment);
	xml->get_widget("graph_win_status_bar", _status_bar);
	xml->get_widget("graph_import_menuitem", _menu_import);
	xml->get_widget("graph_save_menuitem", _menu_save);
	xml->get_widget("graph_save_as_menuitem", _menu_save_as);
	xml->get_widget("graph_export_image_menuitem", _menu_export_image);
	xml->get_widget("graph_redo_menuitem", _menu_redo);
	xml->get_widget("graph_undo_menuitem", _menu_undo);
	xml->get_widget("graph_cut_menuitem", _menu_cut);
	xml->get_widget("graph_copy_menuitem", _menu_copy);
	xml->get_widget("graph_paste_menuitem", _menu_paste);
	xml->get_widget("graph_delete_menuitem", _menu_delete);
	xml->get_widget("graph_select_all_menuitem", _menu_select_all);
	xml->get_widget("graph_close_menuitem", _menu_close);
	xml->get_widget("graph_quit_menuitem", _menu_quit);
	xml->get_widget("graph_view_control_window_menuitem", _menu_view_control_window);
	xml->get_widget("graph_view_engine_window_menuitem", _menu_view_engine_window);
	xml->get_widget("graph_properties_menuitem", _menu_view_graph_properties);
	xml->get_widget("graph_parent_menuitem", _menu_parent);
	xml->get_widget("graph_refresh_menuitem", _menu_refresh);
	xml->get_widget("graph_fullscreen_menuitem", _menu_fullscreen);
	xml->get_widget("graph_animate_signals_menuitem", _menu_animate_signals);
	xml->get_widget("graph_sprung_layout_menuitem", _menu_sprung_layout);
	xml->get_widget("graph_human_names_menuitem", _menu_human_names);
	xml->get_widget("graph_show_port_names_menuitem", _menu_show_port_names);
	xml->get_widget("graph_zoom_in_menuitem", _menu_zoom_in);
	xml->get_widget("graph_zoom_out_menuitem", _menu_zoom_out);
	xml->get_widget("graph_zoom_normal_menuitem", _menu_zoom_normal);
	xml->get_widget("graph_zoom_full_menuitem", _menu_zoom_full);
	xml->get_widget("graph_increase_font_size_menuitem", _menu_increase_font_size);
	xml->get_widget("graph_decrease_font_size_menuitem", _menu_decrease_font_size);
	xml->get_widget("graph_normal_font_size_menuitem", _menu_normal_font_size);
	xml->get_widget("graph_doc_pane_menuitem", _menu_show_doc_pane);
	xml->get_widget("graph_status_bar_menuitem", _menu_show_status_bar);
	xml->get_widget("graph_arrange_menuitem", _menu_arrange);
	xml->get_widget("graph_view_messages_window_menuitem", _menu_view_messages_window);
	xml->get_widget("graph_view_graph_tree_window_menuitem", _menu_view_graph_tree_window);
	xml->get_widget("graph_help_about_menuitem", _menu_help_about);
	xml->get_widget("graph_documentation_paned", _doc_paned);
	xml->get_widget("graph_documentation_scrolledwindow", _doc_scrolledwindow);

	_menu_view_control_window->property_sensitive() = false;
	_menu_import->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_import));
	_menu_save->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_save));
	_menu_save_as->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_save_as));
	_menu_export_image->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_export_image));
	_menu_redo->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_redo));
	_menu_undo->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_undo));
	_menu_copy->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_copy));
	_menu_paste->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_paste));
	_menu_delete->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_delete));
	_menu_select_all->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_select_all));
	_menu_close->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_close));
	_menu_quit->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_quit));
	_menu_parent->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_parent_activated));
	_menu_refresh->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_refresh_activated));
	_menu_fullscreen->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_fullscreen_toggled));
	_menu_animate_signals->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_animate_signals_toggled));
	_menu_sprung_layout->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_sprung_layout_toggled));
	_menu_human_names->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_human_names_toggled));
	_menu_show_doc_pane->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_doc_pane_toggled));
	_menu_show_status_bar->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_status_bar_toggled));
	_menu_show_port_names->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_port_names_toggled));
	_menu_arrange->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_arrange));
	_menu_zoom_in->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_zoom_in));
	_menu_zoom_out->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_zoom_out));
	_menu_zoom_normal->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_zoom_normal));
	_menu_zoom_full->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_zoom_full));
	_menu_increase_font_size->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_increase_font_size));
	_menu_decrease_font_size->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_decrease_font_size));
	_menu_normal_font_size->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_normal_font_size));
	_menu_view_engine_window->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_show_engine));
	_menu_view_graph_properties->signal_activate().connect(
		sigc::mem_fun(this, &GraphBox::event_show_properties));

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->signal_owner_change().connect(
		sigc::mem_fun(this, &GraphBox::event_clipboard_changed));

#ifdef __APPLE__
	_menu_paste->set_sensitive(true);
#endif

	_status_label = Gtk::manage(new Gtk::Label("STATUS"));
	_status_bar->pack_start(*_status_label, false, true, 0);
	_status_label->show();
}

GraphBox::~GraphBox()
{
	delete _breadcrumbs;
}

SPtr<GraphBox>
GraphBox::create(App& app, SPtr<const GraphModel> graph)
{
	GraphBox* result = nullptr;
	Glib::RefPtr<Gtk::Builder> xml = WidgetFactory::create("graph_win");
	xml->get_widget_derived("graph_win_vbox", result);
	result->init_box(app);
	result->set_graph(graph, SPtr<GraphView>());

	if (app.is_plugin()) {
		result->_menu_close->set_sensitive(false);
		result->_menu_quit->set_sensitive(false);
	}

	return SPtr<GraphBox>(result);
}

void
GraphBox::init_box(App& app)
{
	_app = &app;

	const URI engine_uri(_app->interface()->uri());
	if (engine_uri == "ingen:/clients/event_writer") {
		_status_bar->push("Running internal engine", STATUS_CONTEXT_ENGINE);
	} else {
		_status_bar->push(
			(fmt("Connected to %1%") % engine_uri).str(),
			STATUS_CONTEXT_ENGINE);
	}

	_menu_view_messages_window->signal_activate().connect(
		sigc::mem_fun<void>(_app->messages_dialog(), &MessagesWindow::present));
	_menu_view_graph_tree_window->signal_activate().connect(
		sigc::mem_fun<void>(_app->graph_tree(), &GraphTreeWindow::present));

	_menu_help_about->signal_activate().connect(
		sigc::hide_return(sigc::mem_fun(_app, &App::show_about)));

	_breadcrumbs = new BreadCrumbs(*_app);
	_breadcrumbs->signal_graph_selected.connect(
		sigc::mem_fun(this, &GraphBox::set_graph_from_path));

	_status_label->set_markup(app.status_text());
	app.signal_status_text_changed.connect(
		sigc::mem_fun(*this, &GraphBox::set_status_text));
}

void
GraphBox::set_status_text(const std::string& text)
{
	_status_label->set_markup(text);
}

void
GraphBox::set_graph_from_path(const Raul::Path& path, SPtr<GraphView> view)
{
	if (view) {
		assert(view->graph()->path() == path);
		_app->window_factory()->present_graph(view->graph(), _window, view);
	} else {
		SPtr<const GraphModel> model = dynamic_ptr_cast<const GraphModel>(
			_app->store()->object(path));
		if (model) {
			_app->window_factory()->present_graph(model, _window);
		}
	}
}

/** Sets the graph for this box and initializes everything.
 *
 * If `view` is NULL, a new view will be created.
 */
void
GraphBox::set_graph(SPtr<const GraphModel> graph,
                    SPtr<GraphView>        view)
{
	if (!graph || graph == _graph) {
		return;
	}

	_enable_signal = false;

	new_port_connection.disconnect();
	removed_port_connection.disconnect();
	edit_mode_connection.disconnect();
	_entered_connection.disconnect();
	_left_connection.disconnect();

	_status_bar->pop(STATUS_CONTEXT_GRAPH);

	_graph = graph;
	_view  = view;

	if (!_view) {
		_view = _breadcrumbs->view(graph->path());
	}

	if (!_view) {
		_view = GraphView::create(*_app, graph);
	}

	assert(_view);

	graph->signal_property().connect(
		sigc::mem_fun(this, &GraphBox::property_changed));

	if (!_view->canvas()->supports_sprung_layout()) {
		_menu_sprung_layout->set_active(false);
		_menu_sprung_layout->set_sensitive(false);
	}

	// Add view to our alignment
	if (_view->get_parent()) {
		_view->get_parent()->remove(*_view.get());
	}

	_alignment->remove();
	_alignment->add(*_view.get());

	if (_breadcrumbs->get_parent()) {
		_breadcrumbs->get_parent()->remove(*_breadcrumbs);
	}

	_view->breadcrumb_container()->remove();
	_view->breadcrumb_container()->add(*_breadcrumbs);
	_view->breadcrumb_container()->show();

	_breadcrumbs->build(graph->path(), _view);
	_breadcrumbs->show();

	_menu_view_control_window->property_sensitive() = false;

	for (const auto& p : graph->ports()) {
		if (_app->can_control(p.get())) {
			_menu_view_control_window->property_sensitive() = true;
			break;
		}
	}

	_menu_parent->property_sensitive() = bool(graph->parent());

	new_port_connection = graph->signal_new_port().connect(
		sigc::mem_fun(this, &GraphBox::graph_port_added));
	removed_port_connection = graph->signal_removed_port().connect(
		sigc::mem_fun(this, &GraphBox::graph_port_removed));

	show();
	_alignment->show_all();

	_menu_human_names->set_active(
		_app->world()->conf().option("human-names").get<int32_t>());
	_menu_show_port_names->set_active(
		_app->world()->conf().option("port-labels").get<int32_t>());

	_doc_paned->set_position(std::numeric_limits<int>::max());
	_doc_scrolledwindow->hide();

	_enable_signal = true;
}

void
GraphBox::graph_port_added(SPtr<const PortModel> port)
{
	if (port->is_input() && _app->can_control(port.get())) {
		_menu_view_control_window->property_sensitive() = true;
	}
}

void
GraphBox::graph_port_removed(SPtr<const PortModel> port)
{
	if (!(port->is_input() && _app->can_control(port.get()))) {
		return;
	}

	for (const auto& p : _graph->ports()) {
		if (p->is_input() && _app->can_control(p.get())) {
			_menu_view_control_window->property_sensitive() = true;
			return;
		}
	}

	_menu_view_control_window->property_sensitive() = false;
}

void
GraphBox::property_changed(const URI& predicate, const Atom& value)
{
	if (predicate == _app->uris().ingen_sprungLayout) {
		if (value.type() == _app->uris().forge.Bool) {
			_menu_sprung_layout->set_active(value.get<int32_t>());
		}
	}
}

void
GraphBox::set_documentation(const std::string& doc, bool html)
{
	_doc_scrolledwindow->remove();
	if (doc.empty()) {
		_doc_scrolledwindow->hide();
		return;
	}
#ifdef HAVE_WEBKIT
	WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	webkit_web_view_load_html_string(view, doc.c_str(), "");
	Gtk::Widget* widget = Gtk::manage(Glib::wrap(GTK_WIDGET(view)));
	_doc_scrolledwindow->add(*widget);
	widget->show();
#else
	Gtk::TextView* view = Gtk::manage(new Gtk::TextView());
	view->get_buffer()->set_text(doc);
	view->set_wrap_mode(Gtk::WRAP_WORD);
	_doc_scrolledwindow->add(*view);
	view->show();
#endif
}

void
GraphBox::show_status(const ObjectModel* model)
{
	std::stringstream msg;
	msg << model->path();

	const PortModel*  port = nullptr;
	const BlockModel* block = nullptr;

	if ((port = dynamic_cast<const PortModel*>(model))) {
		show_port_status(port, port->value());

	} else if ((block = dynamic_cast<const BlockModel*>(model))) {
		const PluginModel* plugin = dynamic_cast<const PluginModel*>(block->plugin());
		if (plugin) {
			msg << ((boost::format(" (%1%)") % plugin->human_name()).str());
		}
		_status_bar->push(msg.str(), STATUS_CONTEXT_HOVER);
	}
}

void
GraphBox::show_port_status(const PortModel* port, const Atom& value)
{
	std::stringstream msg;
	msg << port->path();

	const BlockModel* parent = dynamic_cast<const BlockModel*>(port->parent().get());
	if (parent) {
		const PluginModel* plugin = dynamic_cast<const PluginModel*>(parent->plugin());
		if (plugin) {
			const std::string& human_name = plugin->port_human_name(port->index());
			if (!human_name.empty()) {
				msg << " (" << human_name << ")";
			}
		}
	}

	if (value.is_valid()) {
		msg << " = " << _app->forge().str(value, true);
	}

	_status_bar->pop(STATUS_CONTEXT_HOVER);
	_status_bar->push(msg.str(), STATUS_CONTEXT_HOVER);
}

void
GraphBox::object_entered(const ObjectModel* model)
{
	show_status(model);
}

void
GraphBox::object_left(const ObjectModel* model)
{
	_status_bar->pop(STATUS_CONTEXT_GRAPH);
	_status_bar->pop(STATUS_CONTEXT_HOVER);
}

void
GraphBox::event_show_engine()
{
	if (_graph) {
		_app->connect_window()->show();
	}
}

void
GraphBox::event_clipboard_changed(GdkEventOwnerChange* ev)
{
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	_menu_paste->set_sensitive(clipboard->wait_is_text_available());
}

void
GraphBox::event_show_properties()
{
	_app->window_factory()->present_properties(_graph);
}

void
GraphBox::event_import()
{
	_app->window_factory()->present_load_graph(_graph);
}

void
GraphBox::event_save()
{
	const Atom& document = _graph->get_property(_app->uris().ingen_file);
	if (!document.is_valid() || document.type() != _app->uris().forge.URI) {
		event_save_as();
	} else {
		save_graph(URI(document.ptr<char>()));
	}
}

void
GraphBox::error(const Glib::ustring& message,
                const Glib::ustring& secondary_text)
{
	Gtk::MessageDialog dialog(
		message, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	dialog.set_secondary_text(secondary_text);
	if (_window) {
		dialog.set_transient_for(*_window);
	}
	dialog.run();
}

bool
GraphBox::confirm(const Glib::ustring& message,
                  const Glib::ustring& secondary_text)
{
	Gtk::MessageDialog dialog(
		message, true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
	dialog.set_secondary_text(secondary_text);
	if (_window) {
		dialog.set_transient_for(*_window);
	}
	return dialog.run() == Gtk::RESPONSE_YES;
}

void
GraphBox::save_graph(const URI& uri)
{
	if (_app->interface()->uri().string().substr(0, 3) == "tcp") {
		_status_bar->push(
			(boost::format("Saved %1% to %2% on client")
			 % _graph->path() % uri).str(),
			STATUS_CONTEXT_GRAPH);
		_app->loader()->save_graph(_graph, uri);
	} else {
		_status_bar->push(
			(boost::format("Saved %1% to %2% on server")
			 % _graph->path() % uri).str(),
			STATUS_CONTEXT_GRAPH);
		_app->interface()->copy(_graph->uri(), uri);
	}
}

void
GraphBox::event_save_as()
{
	const URIs& uris = _app->uris();
	while (true) {
		Gtk::FileChooserDialog dialog(
			"Save Graph", Gtk::FILE_CHOOSER_ACTION_SAVE);
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
		const Atom& document = _graph->get_property(uris.ingen_file);
		const Atom& dir      = _app->world()->conf().option("graph-directory");
		if (document.type() == uris.forge.URI) {
			dialog.set_uri(document.ptr<char>());
		} else if (dir.is_valid()) {
			dialog.set_current_folder(dir.ptr<char>());
		}

		if (dialog.run() != Gtk::RESPONSE_OK) {
			break;
		}

		std::string filename = dialog.get_filename();
		std::string basename = Glib::path_get_basename(filename);

		if (basename.find('.') == std::string::npos) {
			filename += ".ingen";
			basename += ".ingen";
		} else if (filename.substr(filename.length() - 4) == ".ttl") {
			const Glib::ustring dir = Glib::path_get_dirname(filename);
			if (dir.substr(dir.length() - 6) != ".ingen") {
				error("<b>File does not appear to be in an Ingen bundle.");
			}
		} else if (filename.substr(filename.length() - 6) != ".ingen") {
			error("<b>Ingen bundles must end in \".ingen\"</b>");
			continue;
		}

		const std::string symbol(basename.substr(0, basename.find('.')));

		if (!Raul::Symbol::is_valid(symbol)) {
			error(
				"<b>Ingen bundle names must be valid symbols.</b>",
				"All characters must be _, a-z, A-Z, or 0-9, but the first may not be 0-9.");
			continue;
		}

		//_graph->set_property(uris.lv2_symbol, Atom(symbol.c_str()));

		bool confirmed = true;
		if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
			if (Glib::file_test(Glib::build_filename(filename, "manifest.ttl"),
			                    Glib::FILE_TEST_EXISTS)) {
				confirmed = confirm(
					(boost::format("<b>The bundle \"%1%\" already exists."
					               "  Replace it?</b>") % basename).str());
			} else {
				confirmed = confirm(
					(boost::format("<b>A directory named \"%1%\" already exists,"
					               "but is not an Ingen bundle.  "
					               "Save into it anyway?</b>") % basename).str(),
					"This will create at least 2 .ttl files in this directory,"
					"and possibly several more files and/or directories, recursively.  "
					"Existing files will be overwritten.");
			}
		} else if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
			confirmed = confirm(
				(boost::format("<b>A file named \"%1%\" already exists.  "
				               "Replace it with an Ingen bundle?</b>")
				 % basename).str());
			if (confirmed) {
				::g_remove(filename.c_str());
			}
		}

		if (confirmed) {
			const Glib::ustring uri = Glib::filename_to_uri(filename);
			save_graph(URI(uri));

			const_cast<GraphModel*>(_graph.get())->set_property(
				uris.ingen_file,
				_app->forge().alloc_uri(uri.c_str()));
		}

		_app->world()->conf().set(
			"graph-directory",
			_app->world()->forge().alloc(dialog.get_current_folder()));

		break;
	}
}

void
GraphBox::event_export_image()
{
	Gtk::FileChooserDialog dialog("Export Image", Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	dialog.set_default_response(Gtk::RESPONSE_OK);
	if (_window) {
		dialog.set_transient_for(*_window);
	}

	typedef std::map<std::string, std::string> Types;
	Types types;
	types["*.dot"] = "Graphviz DOT";
	types["*.pdf"] = "Portable Document Format";
	types["*.ps"]  = "PostScript";
	types["*.svg"] = "Scalable Vector Graphics";
	for (Types::const_iterator t = types.begin(); t != types.end(); ++t) {
		Gtk::FileFilter filt;
		filt.add_pattern(t->first);
		filt.set_name(t->second);
		dialog.add_filter(filt);
		if (t->first == "*.pdf") {
			dialog.set_filter(filt);
		}
	}

	Gtk::CheckButton* bg_but = new Gtk::CheckButton("Draw _Background", true);
	Gtk::Alignment*   extra  = new Gtk::Alignment(1.0, 0.5, 0.0, 0.0);
	bg_but->set_active(true);
	extra->add(*Gtk::manage(bg_but));
	extra->show_all();
	dialog.set_extra_widget(*Gtk::manage(extra));

	if (dialog.run() == Gtk::RESPONSE_OK) {
		const std::string filename = dialog.get_filename();
		if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
			Gtk::MessageDialog confirm(
				std::string("File exists!  Overwrite ") + filename + "?",
				true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			confirm.set_transient_for(dialog);
			if (confirm.run() != Gtk::RESPONSE_YES) {
				return;
			}
		}
		_view->canvas()->export_image(filename.c_str(), bg_but->get_active());
		_status_bar->push((boost::format("Rendered %1% to %2%")
		                   % _graph->path() % filename).str(),
		                  STATUS_CONTEXT_GRAPH);
	}
}

void
GraphBox::event_copy()
{
	if (_view) {
		_view->canvas()->copy_selection();
	}
}

void
GraphBox::event_redo()
{
	_app->interface()->redo();
}

void
GraphBox::event_undo()
{
	_app->interface()->undo();
}

void
GraphBox::event_paste()
{
	if (_view) {
		_view->canvas()->paste();
	}
}

void
GraphBox::event_delete()
{
	if (_view) {
		_view->canvas()->destroy_selection();
	}
}

void
GraphBox::event_select_all()
{
	if (_view) {
		_view->canvas()->select_all();
	}
}

void
GraphBox::event_close()
{
	if (_window) {
		_app->window_factory()->remove_graph_window(_window);
	}
}

void
GraphBox::event_quit()
{
	_app->quit(_window);
}

void
GraphBox::event_zoom_in()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() + 1.0);
}

void
GraphBox::event_zoom_out()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() - 1.0);
}

void
GraphBox::event_zoom_normal()
{
	_view->canvas()->set_zoom(1.0);
}

void
GraphBox::event_zoom_full()
{
	_view->canvas()->zoom_full();
}

void
GraphBox::event_increase_font_size()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() + 1.0);
}
void
GraphBox::event_decrease_font_size()
{
	_view->canvas()->set_font_size(_view->canvas()->get_font_size() - 1.0);
}
void
GraphBox::event_normal_font_size()
{
	_view->canvas()->set_font_size(_view->canvas()->get_default_font_size());
}

void
GraphBox::event_arrange()
{
	_app->interface()->bundle_begin();
	_view->canvas()->arrange();
	_app->interface()->bundle_end();
}

void
GraphBox::event_parent_activated()
{
	SPtr<client::GraphModel> parent = dynamic_ptr_cast<client::GraphModel>(_graph->parent());
	if (parent) {
		_app->window_factory()->present_graph(parent, _window);
	}
}

void
GraphBox::event_refresh_activated()
{
	_app->interface()->get(_graph->uri());
}

void
GraphBox::event_fullscreen_toggled()
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
GraphBox::event_doc_pane_toggled()
{
	if (_menu_show_doc_pane->get_active()) {
		_doc_scrolledwindow->show_all();
		if (!_has_shown_documentation) {
			const Gtk::Allocation allocation = get_allocation();
			_doc_paned->set_position(allocation.get_width() / 1.61803399);
			_has_shown_documentation = true;
		}
	} else {
		_doc_scrolledwindow->hide();
	}
}

void
GraphBox::event_status_bar_toggled()
{
	if (_menu_show_status_bar->get_active()) {
		_status_bar->show();
	} else {
		_status_bar->hide();
	}
}

void
GraphBox::event_animate_signals_toggled()
{
	_app->interface()->set_property(
		URI("ingen:/clients/this"),
		_app->uris().ingen_broadcast,
		_app->forge().make((bool)_menu_animate_signals->get_active()));
}

void
GraphBox::event_sprung_layout_toggled()
{
	const bool sprung = _menu_sprung_layout->get_active();

	_view->canvas()->set_sprung_layout(sprung);

	Properties properties;
	properties.emplace(_app->uris().ingen_sprungLayout,
	                   _app->forge().make(sprung));
	_app->interface()->put(_graph->uri(), properties);
}

void
GraphBox::event_human_names_toggled()
{
	_view->canvas()->show_human_names(_menu_human_names->get_active());
	_app->world()->conf().set(
		"human-names",
		_app->world()->forge().make(_menu_human_names->get_active()));
}

void
GraphBox::event_port_names_toggled()
{
	_app->world()->conf().set(
		"port-labels",
		_app->world()->forge().make(_menu_show_port_names->get_active()));
	if (_menu_show_port_names->get_active()) {
		_view->canvas()->set_direction(GANV_DIRECTION_RIGHT);
		_view->canvas()->show_port_names(true);
	} else {
		_view->canvas()->set_direction(GANV_DIRECTION_DOWN);
		_view->canvas()->show_port_names(false);
	}
}

} // namespace gui
} // namespace ingen
