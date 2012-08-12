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
#include <string>

#include <gtkmm/eventbox.h>

#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include "ingen/Interface.hpp"
#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/client/PluginUI.hpp"
#include "raul/Atom.hpp"
#include "raul/log.hpp"

#include "App.hpp"
#include "Configuration.hpp"
#include "NodeMenu.hpp"
#include "NodeModule.hpp"
#include "PatchCanvas.hpp"
#include "PatchWindow.hpp"
#include "Port.hpp"
#include "RenameWindow.hpp"
#include "SubpatchModule.hpp"
#include "WidgetFactory.hpp"
#include "WindowFactory.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Client;

namespace GUI {

NodeModule::NodeModule(PatchCanvas&               canvas,
                       SharedPtr<const NodeModel> node)
	: Ganv::Module(canvas, node->path().symbol(), 0, 0, true)
	, _node(node)
	, _gui_widget(NULL)
	, _gui_window(NULL)
{
	assert(_node);

	node->signal_new_port().connect(
		sigc::mem_fun(this, &NodeModule::new_port_view));
	node->signal_removed_port().connect(
		sigc::hide_return(sigc::mem_fun(this, &NodeModule::delete_port_view)));
	node->signal_property().connect(
		sigc::mem_fun(this, &NodeModule::property_changed));
	node->signal_moved().connect(
		sigc::mem_fun(this, &NodeModule::rename));

	signal_event().connect(
		sigc::mem_fun(this, &NodeModule::on_event));

	signal_moved().connect(
		sigc::mem_fun(this, &NodeModule::store_location));

	const PluginModel* plugin = dynamic_cast<const PluginModel*>(node->plugin());
	if (plugin) {
		plugin->signal_changed().connect(
			sigc::mem_fun(this, &NodeModule::plugin_changed));
	}
}

NodeModule::~NodeModule()
{
	delete _gui_widget;
	delete _gui_window;
}

bool
NodeModule::show_menu(GdkEventButton* ev)
{
	WidgetFactory::get_widget_derived("object_menu", _menu);
	_menu->init(app(), _node);
	_menu->signal_embed_gui.connect(
		sigc::mem_fun(this, &NodeModule::on_embed_gui_toggled));
	_menu->signal_popup_gui.connect(
		sigc::hide_return(sigc::mem_fun(this, &NodeModule::popup_gui)));
	_menu->popup(ev->button, ev->time);
	return true;
}

NodeModule*
NodeModule::create(PatchCanvas&               canvas,
                   SharedPtr<const NodeModel> node,
                   bool                       human)
{
	NodeModule* ret;

	SharedPtr<const PatchModel> patch = PtrCast<const PatchModel>(node);
	if (patch)
		ret = new SubpatchModule(canvas, patch);
	else
		ret = new NodeModule(canvas, node);

	for (GraphObject::Properties::const_iterator m = node->properties().begin();
	     m != node->properties().end(); ++m)
		ret->property_changed(m->first, m->second);

	for (NodeModel::Ports::const_iterator p = node->ports().begin();
	     p != node->ports().end(); ++p)
		ret->new_port_view(*p);

	ret->set_stacked(node->polyphonic());

	if (human)
		ret->show_human_names(human); // FIXME: double port iteration

	return ret;
}

App&
NodeModule::app() const
{
	return ((PatchCanvas*)canvas())->app();
}

void
NodeModule::show_human_names(bool b)
{
	const URIs& uris = app().uris();

	if (b) {
		set_label(node()->label().c_str());
	} else {
		set_label(node()->symbol().c_str());
	}

	for (iterator i = begin(); i != end(); ++i) {
		Ingen::GUI::Port* const port = dynamic_cast<Ingen::GUI::Port*>(*i);
		Glib::ustring label(port->model()->symbol().c_str());
		if (b) {
			const Raul::Atom& name_property = port->model()->get_property(uris.lv2_name);
			if (name_property.type() == uris.forge.String) {
				label = name_property.get_string();
			} else {
				Glib::ustring hn = node()->plugin_model()->port_human_name(
						port->model()->index());
				if (!hn.empty())
					label = hn;
			}
		}
		(*i)->set_label(label.c_str());
	}
}

void
NodeModule::value_changed(uint32_t index, const Atom& value)
{
	if (!_plugin_ui)
		return;

	float       float_val = 0.0f;
	const URIs& uris      = app().uris();

	if (value.type() == uris.forge.Float) {
		float_val = value.get_float();
		_plugin_ui->port_event(index, 4, 0, &float_val);
	} else if (value.type() == uris.forge.String) {
		_plugin_ui->port_event(
			index, strlen(value.get_string()), 0, value.get_string());
	}
}

void
NodeModule::port_activity(uint32_t index, const Atom& value)
{
	if (!_plugin_ui)
		return;

	const URIs& uris = app().uris();

	// FIXME: Well, this sucks...
	LV2_Atom* atom = (LV2_Atom*)malloc(sizeof(LV2_Atom) + value.size());
	atom->type = value.type();
	atom->size = value.type();
	memcpy(LV2_ATOM_BODY(atom), value.get_body(), value.size());
	_plugin_ui->port_event(
		index, lv2_atom_total_size(atom), uris.atom_eventTransfer, atom);
	free(atom);
}

void
NodeModule::plugin_changed()
{
	for (iterator p = begin(); p !=end(); ++p)
		dynamic_cast<Ingen::GUI::Port*>(*p)->update_metadata();
}

void
NodeModule::on_embed_gui_toggled(bool embed)
{
	embed_gui(embed);
	app().interface()->set_property(_node->uri(),
	                                app().uris().ingen_uiEmbedded,
	                                app().forge().make(embed));
}

void
NodeModule::embed_gui(bool embed)
{
	if (embed) {
		if (_gui_window) {
			warn << "LV2 GUI already popped up, cannot embed" << endl;
			return;
		}

		if (!_plugin_ui) {
			const PluginModel* const pm = dynamic_cast<const PluginModel*>(_node->plugin());
			_plugin_ui = pm->ui(app().world(), _node);
		}

		if (_plugin_ui) {
			GtkWidget* c_widget = (GtkWidget*)_plugin_ui->get_widget();
			_gui_widget = Glib::wrap(c_widget);

			Gtk::Container* container = new Gtk::EventBox();
			container->set_name("ingen_embedded_node_gui_container");
			container->set_border_width(2.0);
			container->add(*_gui_widget);
			Ganv::Module::embed(container);
		} else {
			error << "Failed to create LV2 UI" << endl;
		}

		if (_gui_widget) {
			_gui_widget->show_all();
			set_control_values();
		}

	} else { // un-embed
		Ganv::Module::embed(NULL);
		_plugin_ui.reset();
	}
}

void
NodeModule::rename()
{
	if (app().configuration()->name_style() == Configuration::PATH) {
		set_label(_node->path().symbol());
	}
}

void
NodeModule::new_port_view(SharedPtr<const PortModel> port)
{
	Port::create(app(), *this, port,
	             app().configuration()->name_style() == Configuration::HUMAN);

	port->signal_value_changed().connect(
		sigc::bind<0>(sigc::mem_fun(this, &NodeModule::value_changed),
		              port->index()));

	port->signal_activity().connect(
		sigc::bind<0>(sigc::mem_fun(this, &NodeModule::port_activity),
		              port->index()));
}

Port*
NodeModule::port(boost::shared_ptr<const PortModel> model)
{
	for (iterator p = begin(); p != end(); ++p) {
		Port* const port = dynamic_cast<Port*>(*p);
		if (port->model() == model)
			return port;
	}
	return NULL;
}

void
NodeModule::delete_port_view(SharedPtr<const PortModel> model)
{
	Port* p = port(model);
	if (p) {
		delete p;
	} else {
		warn << "Failed to find port on module " << model->path() << endl;
	}
}

bool
NodeModule::popup_gui()
{
	if (_node->plugin() && _node->plugin()->type() == PluginModel::LV2) {
		if (_plugin_ui) {
			warn << "LV2 GUI already embedded, cannot pop up" << endl;
			return false;
		}

		const PluginModel* const plugin = dynamic_cast<const PluginModel*>(_node->plugin());
		assert(plugin);

		_plugin_ui = plugin->ui(app().world(), _node);

		if (_plugin_ui) {
			GtkWidget* c_widget = (GtkWidget*)_plugin_ui->get_widget();
			_gui_widget = Glib::wrap(c_widget);

			_gui_window = new Gtk::Window();
			if (!_plugin_ui->is_resizable()) {
				_gui_window->set_resizable(false);
			}
			_gui_window->set_title(_node->path().str() + " UI - Ingen");
			_gui_window->set_role("plugin_ui");
			_gui_window->add(*_gui_widget);
			_gui_widget->show_all();
			set_control_values();

			_gui_window->signal_unmap().connect(
					sigc::mem_fun(this, &NodeModule::on_gui_window_close));
			_gui_window->present();

			return true;
		} else {
			warn << "No LV2 GUI for " << _node->path() << endl;
		}
	}

	return false;
}

void
NodeModule::on_gui_window_close()
{
	delete _gui_window;
	_gui_window = NULL;
	_plugin_ui.reset();
	_gui_widget = NULL;
}

void
NodeModule::set_control_values()
{
	uint32_t index = 0;
	for (NodeModel::Ports::const_iterator p = _node->ports().begin();
	     p != _node->ports().end(); ++p) {
		if (app().can_control(p->get()))
			value_changed(index, (*p)->value());
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
	} else if (ev->type == GDK_2BUTTON_PRESS) {
		return on_double_click(&ev->button);
	}
	return false;
}

void
NodeModule::store_location(double ax, double ay)
{
	const Atom x(app().forge().make(static_cast<float>(ax)));
	const Atom y(app().forge().make(static_cast<float>(ay)));

	const URIs& uris = app().uris();

	const Atom& existing_x = _node->get_property(uris.ingen_canvasX);
	const Atom& existing_y = _node->get_property(uris.ingen_canvasY);

	if (x != existing_x && y != existing_y) {
		Resource::Properties remove;
		remove.insert(make_pair(uris.ingen_canvasX, uris.wildcard));
		remove.insert(make_pair(uris.ingen_canvasY, uris.wildcard));
		Resource::Properties add;
		add.insert(make_pair(uris.ingen_canvasX, x));
		add.insert(make_pair(uris.ingen_canvasY, y));
		app().interface()->delta(_node->uri(), remove, add);
	}
}

void
NodeModule::property_changed(const URI& key, const Atom& value)
{
	const URIs& uris = app().uris();
	if (value.type() == uris.forge.Float) {
		if (key == uris.ingen_canvasX) {
			move_to(value.get_float(), get_y());
		} else if (key == uris.ingen_canvasY) {
			move_to(get_x(), value.get_float());
		}
	} else if (value.type() == uris.forge.Bool) {
		if (key == uris.ingen_polyphonic) {
			set_stacked(value.get_bool());
		} else if (key == uris.ingen_uiEmbedded) {
			if (value.get_bool() && !_gui_widget) {
				embed_gui(true);
			} else if (!value.get_bool() && _gui_widget) {
				embed_gui(false);
			}
		}
	} else if (value.type() == uris.forge.String) {
		if (key == uris.lv2_name
		    && app().configuration()->name_style() == Configuration::HUMAN) {
			set_label(value.get_string());
		}
	}
}

void
NodeModule::set_selected(gboolean b)
{
	if (b != get_selected()) {
		Ganv::Module::set_selected(b);
		#if 0
		if (b) {
			PatchWindow* win = app().window_factory()->parent_patch_window(node());
			if (win) {
				std::string doc;
				bool        html = false;
				if (node()->plugin_model()) {
					doc = node()->plugin_model()->documentation(&html);
				}
				if (!doc.empty()) {
					win->show_documentation(doc, html);
				} else {
					win->hide_documentation();
				}
			}
		}
		#endif
	}
}

} // namespace GUI
} // namespace Ingen
