/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NodeModule.hpp"

#include "App.hpp"
#include "GraphBox.hpp"
#include "GraphCanvas.hpp"
#include "GraphWindow.hpp"
#include "NodeMenu.hpp"
#include "Port.hpp"
#include "SubgraphModule.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"
#include "ingen_config.h"

#include <ganv/Module.hpp>
#include <ganv/Port.hpp>
#include <ingen/Atom.hpp>
#include <ingen/Configuration.hpp>
#include <ingen/Forge.hpp>
#include <ingen/Interface.hpp>
#include <ingen/Log.hpp>
#include <ingen/Properties.hpp>
#include <ingen/Resource.hpp>
#include <ingen/URI.hpp>
#include <ingen/URIs.hpp>
#include <ingen/World.hpp>
#include <ingen/client/BlockModel.hpp>
#include <ingen/client/GraphModel.hpp> // IWYU pragma: keep
#include <ingen/client/PluginModel.hpp>
#include <ingen/client/PluginUI.hpp>
#include <ingen/client/PortModel.hpp>
#include <lv2/atom/util.h>
#include <raul/Path.hpp>
#include <raul/Symbol.hpp>

#include <glibmm/main.h>
#include <glibmm/ustring.h>
#include <gtkmm/container.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/widget.h>
#include <gtkmm/window.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/adaptors/retype_return.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/functors/slot.h>
#include <sigc++/signal.h>

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ingen {

using client::BlockModel;
using client::GraphModel;
using client::PluginModel;
using client::PortModel;

namespace gui {

NodeModule::NodeModule(GraphCanvas&                             canvas,
                       const std::shared_ptr<const BlockModel>& block)
    : Ganv::Module(canvas, block->path().symbol(), 0, 0, true)
    , _block(block)
{
	block->signal_new_port().connect(
		sigc::mem_fun(this, &NodeModule::new_port_view));
	block->signal_removed_port().connect(
		sigc::hide_return(sigc::mem_fun(this, &NodeModule::delete_port_view)));
	block->signal_property().connect(
		sigc::mem_fun(this, &NodeModule::property_changed));
	block->signal_moved().connect(
		sigc::mem_fun(this, &NodeModule::rename));

	signal_event().connect(
		sigc::mem_fun(this, &NodeModule::on_event));

	signal_moved().connect(
		sigc::mem_fun(this, &NodeModule::store_location));

	signal_selected().connect(
		sigc::mem_fun(this, &NodeModule::on_selected));

	const auto* plugin = dynamic_cast<const PluginModel*>(block->plugin());
	if (plugin) {
		plugin->signal_changed().connect(
			sigc::mem_fun(this, &NodeModule::plugin_changed));
	}

	for (const auto& p : block->properties()) {
		property_changed(p.first, p.second);
	}

	if (_block->has_property(app().uris().ingen_uiEmbedded,
	                         app().uris().forge.make(true))) {
		// Schedule idle callback to embed GUI once ports arrive
		Glib::signal_timeout().connect(
			sigc::mem_fun(*this, &NodeModule::idle_init), 25, G_PRIORITY_DEFAULT_IDLE);
	} else {
		_initialised = true;
	}
}

NodeModule::~NodeModule()
{
	delete _gui_widget;
	delete _gui_window;
}

bool
NodeModule::idle_init()
{
	if (_block->ports().empty()) {
		return true; // Need to embed GUI, but ports haven't shown up yet
	}

	// Ports have arrived, embed GUI and deregister this callback
	embed_gui(true);
	_initialised = true;
	return false;
}

bool
NodeModule::show_menu(GdkEventButton* ev)
{
	WidgetFactory::get_widget_derived("object_menu", _menu);
	if (!_menu) {
		app().log().error("Failed to load object menu widget\n");
		return false;
	}

	_menu->init(app(), _block);
	_menu->signal_embed_gui.connect(
		sigc::mem_fun(this, &NodeModule::on_embed_gui_toggled));
	_menu->signal_popup_gui.connect(
		sigc::hide_return(sigc::mem_fun(this, &NodeModule::popup_gui)));
	_menu->popup(ev->button, ev->time);
	return true;
}

NodeModule*
NodeModule::create(GraphCanvas&                             canvas,
                   const std::shared_ptr<const BlockModel>& block,
                   bool                                     human)
{
	auto graph = std::dynamic_pointer_cast<const GraphModel>(block);

	NodeModule* ret = (graph)
		? new SubgraphModule(canvas, graph)
		: new NodeModule(canvas, block);

	for (const auto& p : block->properties()) {
		ret->property_changed(p.first, p.second);
	}

	for (const auto& p : block->ports()) {
		ret->new_port_view(p);
	}

	ret->set_stacked(block->polyphonic());

	if (human) {
		ret->show_human_names(human); // FIXME: double port iteration
	}

	return ret;
}

App&
NodeModule::app() const
{
	return static_cast<GraphCanvas*>(canvas())->app();
}

void
NodeModule::show_human_names(bool b)
{
	const URIs& uris = app().uris();

	if (b) {
		set_label(block()->label().c_str());
	} else {
		set_label(block()->symbol().c_str());
	}

	for (auto* p : *this) {
		auto* const port = dynamic_cast<ingen::gui::Port*>(p);
		Glib::ustring label(port->model()->symbol().c_str());
		if (b) {
			const Atom& name_property = port->model()->get_property(uris.lv2_name);
			if (name_property.type() == uris.forge.String) {
				label = name_property.ptr<char>();
			} else {
				const Glib::ustring hn = block()->plugin_model()->port_human_name(
					port->model()->index());
				if (!hn.empty()) {
					label = hn;
				}
			}
		}
		port->set_label(label.c_str());
	}
}

void
NodeModule::port_activity(uint32_t index, const Atom& value)
{
	const URIs& uris = app().uris();
	if (!_plugin_ui) {
		return;
	}

	if (_block->get_port(index)->is_a(uris.atom_AtomPort)) {
		_plugin_ui->port_event(index,
		                       lv2_atom_total_size(value.atom()),
		                       uris.atom_eventTransfer,
		                       value.atom());
	}
}

void
NodeModule::port_value_changed(uint32_t index, const Atom& value)
{
	const URIs& uris = app().uris();
	if (!_plugin_ui) {
		return;
	}

	if (value.type() == uris.atom_Float &&
	    _block->get_port(index)->is_numeric()) {
		_plugin_ui->port_event(index, sizeof(float), 0, value.ptr<float>());
	} else {
		_plugin_ui->port_event(index,
		                       lv2_atom_total_size(value.atom()),
		                       uris.atom_eventTransfer,
		                       value.atom());
	}
}

void
NodeModule::plugin_changed()
{
	for (auto* p : *this) {
		dynamic_cast<ingen::gui::Port*>(p)->update_metadata();
	}
}

void
NodeModule::on_embed_gui_toggled(bool embed)
{
	embed_gui(embed);
	app().set_property(_block->uri(),
	                   app().uris().ingen_uiEmbedded,
	                   app().forge().make(embed));
}

void
NodeModule::embed_gui(bool embed)
{
	if (embed) {
		if (_gui_window) {
			app().log().warn("LV2 GUI already popped up, cannot embed\n");
			return;
		}

		if (!_plugin_ui) {
			_plugin_ui = _block->plugin_model()->ui(app().world(), _block);
		}

		if (_plugin_ui) {
			_plugin_ui->signal_property_changed().connect(
				sigc::mem_fun(app(), &App::set_property));

			if (!_plugin_ui->instantiate()) {
				app().log().error("Failed to instantiate LV2 UI\n");
			} else {
				auto* c_widget =
				    static_cast<GtkWidget*>(_plugin_ui->get_widget());

				_gui_widget = Glib::wrap(c_widget);

				Gtk::Container* container = new Gtk::EventBox();
				container->set_name("IngenEmbeddedUI");
				container->set_border_width(4.0);
				container->add(*_gui_widget);
				Ganv::Module::embed(container);
			}
		} else {
			app().log().error("Failed to create LV2 UI\n");
		}

		if (_gui_widget) {
			_gui_widget->show_all();
			set_control_values();
		}

	} else { // un-embed
		Ganv::Module::embed(nullptr);
		_plugin_ui.reset();
	}
}

void
NodeModule::rename()
{
	if (app().world().conf().option("port-labels").get<int32_t>() &&
	    !app().world().conf().option("human-names").get<int32_t>()) {
		set_label(_block->path().symbol());
	}
}

void
NodeModule::new_port_view(const std::shared_ptr<const PortModel>& port)
{
	Port::create(app(), *this, port);

	port->signal_value_changed().connect(
		sigc::bind<0>(sigc::mem_fun(this, &NodeModule::port_value_changed),
		              port->index()));

	port->signal_activity().connect(
		sigc::bind<0>(sigc::mem_fun(this, &NodeModule::port_activity),
		              port->index()));
}

Port*
NodeModule::port(const std::shared_ptr<const PortModel>& model)
{
	for (auto* p : *this) {
		auto* const port = dynamic_cast<Port*>(p);
		if (port->model() == model) {
			return port;
		}
	}
	return nullptr;
}

void
NodeModule::delete_port_view(const std::shared_ptr<const PortModel>& model)
{
	Port* const p = port(model);

	if (!p) {
		app().log().warn("Failed to find port %1% on module %2%\n",
		                 model->path(), _block->path());
	}

	delete p;
}

bool
NodeModule::popup_gui()
{
	if (_block->plugin() && app().uris().lv2_Plugin == _block->plugin_model()->type()) {
		if (_plugin_ui) {
			_gui_window->present();
			return true;
		}

		const auto* const plugin =
		    dynamic_cast<const PluginModel*>(_block->plugin());

		assert(plugin);

		_plugin_ui = plugin->ui(app().world(), _block);

		if (_plugin_ui) {
			_plugin_ui->signal_property_changed().connect(
				sigc::mem_fun(app(), &App::set_property));

			if (!_plugin_ui->instantiated() &&  !_plugin_ui->instantiate()) {
				app().log().error("Failed to instantiate LV2 UI\n");
				return false;
			}

			auto* c_widget = static_cast<GtkWidget*>(_plugin_ui->get_widget());

			_gui_widget = Glib::wrap(c_widget);

			_gui_window = new Gtk::Window();
			if (!_plugin_ui->is_resizable()) {
				_gui_window->set_resizable(false);
			}
			_gui_window->set_title(_block->path() + " UI - Ingen");
			_gui_window->set_role("plugin_ui");
			_gui_window->add(*_gui_widget);
			_gui_widget->show_all();
			set_control_values();

			_gui_window->signal_unmap().connect(
				sigc::mem_fun(this, &NodeModule::on_gui_window_close));
			_gui_window->present();

			return true;
		}

		app().log().warn("No LV2 GUI for %1%\n", _block->path());
	}

	return false;
}

void
NodeModule::on_gui_window_close()
{
	delete _gui_window;
	_gui_window = nullptr;
	_plugin_ui.reset();
	_gui_widget = nullptr;
}

void
NodeModule::set_control_values()
{
	uint32_t index = 0;
	for (const auto& p : _block->ports()) {
		if (app().can_control(p.get()) && p->value().is_valid()) {
			port_value_changed(index, p->value());
		}
		++index;
	}
}

bool
NodeModule::on_double_click(GdkEventButton* event)
{
	popup_gui();
	return true;
}

bool
NodeModule::on_event(GdkEvent* ev)
{
	if (ev->type == GDK_BUTTON_PRESS && ev->button.button == 3) {
		return show_menu(&ev->button);
	}

	if (ev->type == GDK_2BUTTON_PRESS) {
		return on_double_click(&ev->button);
	}

	if (ev->type == GDK_ENTER_NOTIFY) {
		GraphBox* const box = app().window_factory()->graph_box(
			std::dynamic_pointer_cast<const GraphModel>(_block->parent()));
		if (box) {
			box->object_entered(_block.get());
		}
	} else if (ev->type == GDK_LEAVE_NOTIFY) {
		GraphBox* const box = app().window_factory()->graph_box(
			std::dynamic_pointer_cast<const GraphModel>(_block->parent()));
		if (box) {
			box->object_left(_block.get());
		}
	}

	return false;
}

void
NodeModule::store_location(double ax, double ay)
{
	const URIs& uris = app().uris();

	const Atom x(app().forge().make(static_cast<float>(ax)));
	const Atom y(app().forge().make(static_cast<float>(ay)));

	if (x != _block->get_property(uris.ingen_canvasX) ||
	    y != _block->get_property(uris.ingen_canvasY))
	{
		app().interface()->put(_block->uri(), {{uris.ingen_canvasX, x},
		                                       {uris.ingen_canvasY, y}});
	}
}

void
NodeModule::property_changed(const URI& key, const Atom& value)
{
	const URIs& uris = app().uris();
	if (value.type() == uris.forge.Float) {
		if (key == uris.ingen_canvasX) {
			move_to(static_cast<double>(value.get<float>()), get_y());
		} else if (key == uris.ingen_canvasY) {
			move_to(get_x(), static_cast<double>(value.get<float>()));
		}
	} else if (value.type() == uris.forge.Bool) {
		if (key == uris.ingen_polyphonic) {
			set_stacked(value.get<int32_t>());
		} else if (key == uris.ingen_uiEmbedded && _initialised) {
			if (value.get<int32_t>() && !_gui_widget) {
				embed_gui(true);
			} else if (!value.get<int32_t>() && _gui_widget) {
				embed_gui(false);
			}
		} else if (key == uris.ingen_enabled) {
			if (value.get<int32_t>()) {
				set_dash_length(0.0);
			} else {
				set_dash_length(5.0);
			}
		}
	} else if (value.type() == uris.forge.String) {
		if (key == uris.lv2_name
		    && app().world().conf().option("human-names").get<int32_t>()) {
			set_label(value.ptr<char>());
		}
	}
}

bool
NodeModule::on_selected(gboolean selected)
{
	GraphWindow* win = app().window_factory()->parent_graph_window(block());
	if (!win) {
		return true;
	}

	if (selected && win->documentation_is_visible()) {
		std::string doc;
#if USE_WEBKIT
		const bool html = true;
#else
		const bool html = false;
#endif
		if (block()->plugin_model()) {
			doc = block()->plugin_model()->documentation(html);
		}
		win->set_documentation(doc, html);
	}

	return true;
}

} // namespace gui
} // namespace ingen
