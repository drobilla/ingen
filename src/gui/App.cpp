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

#include "App.hpp"

#include "ConnectWindow.hpp"
#include "GraphTreeWindow.hpp"
#include "MessagesWindow.hpp"
#include "Port.hpp"
#include "RDFS.hpp"
#include "Style.hpp"
#include "ThreadedLoader.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"
#include "rgba.hpp"

#include "ingen/Atom.hpp"
#include "ingen/ColorContext.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/FilePath.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/QueuedInterface.hpp"
#include "ingen/StreamWriter.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/client/SigClientInterface.hpp"
#include "ingen/runtime_paths.hpp"
#include "lilv/lilv.h"
#include "suil/suil.h"

#include <boost/variant/get.hpp>
#include <glib.h>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <glibmm/propertyproxy.h>
#include <gtk/gtk.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/dialog.h>
#include <gtkmm/enums.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/rc.h>
#include <gtkmm/stock.h>
#include <gtkmm/widget.h>
#include <sigc++/functors/mem_fun.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace ingen {
namespace gui {

Gtk::Main* App::_main = nullptr;

App::App(ingen::World& world)
    : _style(new Style(*this))
    , _window_factory(new WindowFactory(*this))
    , _world(world)
{
	_world.conf().load_default("ingen", "gui.ttl");

	WidgetFactory::get_widget_derived("connect_win", _connect_window);
	WidgetFactory::get_widget_derived("messages_win", _messages_window);
	WidgetFactory::get_widget_derived("graph_tree_win", _graph_tree_window);
	WidgetFactory::get_widget("about_win", _about_dialog);
	_connect_window->init_dialog(*this);
	_messages_window->init_window(*this);
	_graph_tree_window->init_window(*this);
	_about_dialog->property_program_name() = "Ingen";
	_about_dialog->property_logo_icon_name() = "ingen";

	client::PluginModel::set_rdf_world(*world.rdf_world());
	client::PluginModel::set_lilv_world(world.lilv_world());

	world.log().set_sink(std::bind(&MessagesWindow::log,
	                               _messages_window,
	                               std::placeholders::_1,
	                               std::placeholders::_2,
	                               std::placeholders::_3));
}

App::~App()
{
	delete _style;
	delete _window_factory;
}

std::shared_ptr<App>
App::create(ingen::World& world)
{
	suil_init(&world.argc(), &world.argv(), SUIL_ARG_NONE);

	// Add RC file for embedded GUI Gtk style
	const std::string rc_path = ingen::data_file_path("ingen_style.rc");
	Gtk::RC::add_default_file(rc_path);

	_main = Gtk::Main::instance();
	if (!_main) {
		Glib::set_application_name("Ingen");
		gtk_window_set_default_icon_name("ingen");
		_main = new Gtk::Main(&world.argc(), &world.argv());
	}

	auto app = std::shared_ptr<App>(new App(world));

	// Load configuration settings
	app->style()->load_settings();
	app->style()->apply_settings();

	// Set default window icon
	app->_about_dialog->property_program_name() = "Ingen";
	app->_about_dialog->property_logo_icon_name() = "ingen";
	gtk_window_set_default_icon_name("ingen");

	return app;
}

void
App::run()
{
	_connect_window->start(*this, world());

	// Run main iterations here until we're attached to the engine. Otherwise
	// with 'ingen -egl' we'd get a bunch of notifications about load
	// immediately before even knowing about the root graph or plugins)
	while (!_connect_window->attached()) {
		if (Gtk::Main::iteration()) {
			break;
		}
	}

	Gtk::Main::run();
}

void
App::attach(const std::shared_ptr<ingen::Interface>& client)
{
	assert(!_client);
	assert(!_store);
	assert(!_loader);

	if (_world.engine()) {
		_world.engine()->register_client(client);
	}

	_client = client;

	_store = std::make_shared<client::ClientStore>(_world.uris(),
	                                               _world.log(),
	                                               sig_client());

	_loader = std::make_shared<ThreadedLoader>(*this, _world.interface());

	if (!_world.store()) {
		_world.set_store(_store);
	}

	if (_world.conf().option("dump").get<int32_t>()) {
		_dumper = std::make_shared<StreamWriter>(_world.uri_map(),
		                                         _world.uris(),
		                                         URI("ingen:/client"),
		                                         stderr,
		                                         ColorContext::Color::CYAN);

		sig_client()->signal_message().connect(
			sigc::mem_fun(*_dumper, &StreamWriter::message));
	}

	_graph_tree_window->init(*this, *_store);
	sig_client()->signal_message().connect(sigc::mem_fun(this, &App::message));
}

void
App::detach()
{
	if (_world.interface()) {
		_window_factory->clear();
		_store->clear();

		_loader.reset();
		_store.reset();
		_client.reset();
		_world.set_interface(nullptr);
	}
}

void
App::request_plugins_if_necessary()
{
	if (!_requested_plugins) {
		_world.interface()->get(URI("ingen:/plugins"));
		_requested_plugins = true;
	}
}

std::shared_ptr<client::SigClientInterface>
App::sig_client()
{
	auto qi = std::dynamic_pointer_cast<QueuedInterface>(_client);
	if (qi) {
		return std::dynamic_pointer_cast<client::SigClientInterface>(qi->sink());
	}
	return std::dynamic_pointer_cast<client::SigClientInterface>(_client);
}

std::shared_ptr<Serialiser>
App::serialiser()
{
	return _world.serialiser();
}

void
App::message(const Message& msg)
{
	if (const Response* const r = boost::get<Response>(&msg)) {
		response(r->id, r->status, r->subject);
	} else if (const Error* const e = boost::get<Error>(&msg)) {
		error_message(e->message);
	} else if (const Put* const p = boost::get<Put>(&msg)) {
		put(p->uri, p->properties, p->ctx);
	} else if (const SetProperty* const s = boost::get<SetProperty>(&msg)) {
		property_change(s->subject, s->predicate, s->value, s->ctx);
	}
}

void
App::response(int32_t id, Status status, const std::string& subject)
{
	if (status != Status::SUCCESS) {
		std::string msg = ingen_status_string(status);
		if (!subject.empty()) {
			msg += ": " + subject;
		}
		error_message(msg);
	}
}

void
App::error_message(const std::string& str)
{
	_messages_window->post_error(str);
}

void
App::set_property(const URI&      subject,
                  const URI&      key,
                  const Atom&     value,
                  Resource::Graph ctx)
{
	// Send message to server
	interface()->set_property(subject, key, value, ctx);

	/* The server does not feed back set messages (kludge to prevent control
	   feedback and bandwidth wastage, see Delta.cpp).  So, assume everything
	   went as planned here and fire the signal ourselves as if the server
	   feedback came back immediately. */
	if (key != uris().ingen_activity) {
		sig_client()->signal_message().emit(SetProperty{0, subject, key, value, ctx});
	}
}

void
App::set_tooltip(Gtk::Widget* widget, const LilvNode* node)
{
	const std::string comment = rdfs::comment(_world, node);
	if (!comment.empty()) {
		widget->set_tooltip_text(comment);
	}
}

void
App::put(const URI&        uri,
         const Properties& properties,
         Resource::Graph   ctx)
{
	_enable_signal = false;
	for (const auto& p : properties) {
		property_change(uri, p.first, p.second);
	}
	_enable_signal = true;
	_status_text   = status_text();
	signal_status_text_changed.emit(_status_text);
}

void
App::property_change(const URI&      subject,
                     const URI&      key,
                     const Atom&     value,
                     Resource::Graph ctx)
{
	if (subject != URI("ingen:/engine")) {
		return;
	} else if (key == uris().param_sampleRate && value.type() == forge().Int) {
		_sample_rate = value.get<int32_t>();
	} else if (key == uris().bufsz_maxBlockLength && value.type() == forge().Int) {
		_block_length = value.get<int32_t>();
	} else if (key == uris().ingen_numThreads && value.type() == forge().Int) {
		_n_threads = value.get<int>();
	} else if (key == uris().ingen_minRunLoad && value.type() == forge().Float) {
		_min_run_load = value.get<float>();
	} else if (key == uris().ingen_meanRunLoad && value.type() == forge().Float) {
		_mean_run_load = value.get<float>();
	} else if (key == uris().ingen_maxRunLoad && value.type() == forge().Float) {
		_max_run_load = value.get<float>();
	} else {
		_world.log().warn("Unknown engine property %1%\n", key);
		return;
	}

	if (_enable_signal) {
		_status_text = status_text();
		signal_status_text_changed.emit(_status_text);
	}
}

static std::string
fraction_label(float f)
{
	static const uint32_t GREEN = 0x4A8A0EFF;
	static const uint32_t RED   = 0x960909FF;

	const uint32_t col = rgba_interpolate(GREEN, RED, std::min(f, 1.0f));
	char           col_str[8];
	snprintf(col_str, sizeof(col_str), "%02X%02X%02X",
	         RGBA_R(col), RGBA_G(col), RGBA_B(col));
	return fmt("<span color='#%s'>%d%%</span>", col_str, (f * 100));
}

std::string
App::status_text() const
{
	return fmt(
		"%2.1f kHz / %.1f ms, %s, %s DSP",
		(_sample_rate / 1e3f),
		(_block_length * 1e3f / static_cast<float>(_sample_rate)),
		((_n_threads == 1) ? "1 thread" : fmt("%1% threads", _n_threads)),
		fraction_label(_max_run_load));
}

void
App::port_activity(Port* port)
{
	const auto inserted = _activity_ports.emplace(port, false);
	if (inserted.second) {
		inserted.first->second = false;
	}

	port->set_highlighted(true);
}

void
App::activity_port_destroyed(Port* port)
{
	auto i = _activity_ports.find(port);
	if (i != _activity_ports.end()) {
		_activity_ports.erase(i);
	}
}

bool
App::animate()
{
	for (auto i = _activity_ports.begin(); i != _activity_ports.end(); ) {
		auto next = i;
		++next;

		if ((*i).second) { // saw it last time, unhighlight and pop
			(*i).first->set_highlighted(false);
			_activity_ports.erase(i);
		} else {
			(*i).second = true;
		}

		i = next;
	}

	return true;
}

/******** Event Handlers ************/

void
App::register_callbacks()
{
	Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &App::gtk_main_iteration), 33, G_PRIORITY_DEFAULT);
}

bool
App::gtk_main_iteration()
{
	if (!_client) {
		return false;
	}

	animate();

	if (_messages_window) {
		_messages_window->flush();
	}

	_enable_signal = false;
	if (_world.engine()) {
		if (!_world.engine()->main_iteration()) {
			Gtk::Main::quit();
			return false;
		}
	} else {
		std::dynamic_pointer_cast<QueuedInterface>(_client)->emit();
	}
	_enable_signal = true;

	return true;
}

void
App::show_about()
{
	_about_dialog->run();
	_about_dialog->hide();
}

/** Prompt (if necessary) and quit application (if confirmed).
 * @return true iff the application quit.
 */
bool
App::quit(Gtk::Window* dialog_parent)
{
	bool quit = true;
	if (_world.engine() && _connect_window->attached()) {
		Gtk::MessageDialog d(
			"The engine is running in this process.  Quitting will terminate Ingen."
			"\n\n" "Are you sure you want to quit?",
			true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
		if (dialog_parent) {
			d.set_transient_for(*dialog_parent);
		}
		d.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		d.add_button(Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE);
		quit = (d.run() == Gtk::RESPONSE_CLOSE);
	}

	if (!quit) {
		return false;
	}

	Gtk::Main::quit();

	try {
		const std::string path = _world.conf().save(
			_world.uri_map(), "ingen", "gui.ttl", Configuration::GUI);
		std::cout << fmt("Saved GUI settings to %1%\n", path);
	} catch (const std::exception& e) {
		std::cerr << fmt("Error saving GUI settings (%1%)\n", e.what());
	}

	return true;
}

bool
App::can_control(const client::PortModel* port) const
{
	return port->is_a(uris().lv2_ControlPort)
		|| port->is_a(uris().lv2_CVPort)
		|| (port->is_a(uris().atom_AtomPort)
		    && (port->supports(uris().atom_Float)
		        || port->supports(uris().atom_String)));
}

uint32_t
App::sample_rate() const
{
	return _sample_rate;
}

} // namespace gui
} // namespace ingen
