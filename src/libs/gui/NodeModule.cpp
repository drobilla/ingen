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

#include <cassert>
#include <raul/Atom.hpp>
#include "interface/EngineInterface.hpp"
#include "client/PatchModel.hpp"
#include "client/NodeModel.hpp"
#include "App.hpp"
#include "NodeModule.hpp"
#include "PatchCanvas.hpp"
#include "Port.hpp"
#include "GladeFactory.hpp"
#include "RenameWindow.hpp"
#include "PatchWindow.hpp"
#include "WindowFactory.hpp"
#include "SubpatchModule.hpp"
#include "NodeControlWindow.hpp"

namespace Ingen {
namespace GUI {


NodeModule::NodeModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node)
	: FlowCanvas::Module(canvas, node->path().name())
	, _node(node)
	, _slv2_ui(NULL)
	, _gui(NULL)
	, _gui_item(NULL)
{
	assert(_node);

	node->signal_new_port.connect(sigc::bind(sigc::mem_fun(this, &NodeModule::add_port), true));
	node->signal_removed_port.connect(sigc::mem_fun(this, &NodeModule::remove_port));
	node->signal_metadata.connect(sigc::mem_fun(this, &NodeModule::set_metadata));
	node->signal_polyphonic.connect(sigc::mem_fun(this, &NodeModule::set_stacked_border));
	node->signal_renamed.connect(sigc::mem_fun(this, &NodeModule::rename));
	
	set_stacked_border(node->polyphonic());
}


NodeModule::~NodeModule()
{
	NodeControlWindow* win = App::instance().window_factory()->control_window(_node);
	
	if (win) {
		// Should remove from window factory via signal
		delete win;
	}
}


void
NodeModule::create_menu()
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget_derived("object_menu", _menu);
	_menu->init(_node);
	_menu->signal_embed_gui.connect(sigc::mem_fun(this, &NodeModule::embed_gui));
	_menu->signal_popup_gui.connect(sigc::hide_return(sigc::mem_fun(this, &NodeModule::popup_gui)));
	
	set_menu(_menu);
}


boost::shared_ptr<NodeModule>
NodeModule::create(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node)
{
	boost::shared_ptr<NodeModule> ret;

	SharedPtr<PatchModel> patch = PtrCast<PatchModel>(node);
	if (patch)
		ret = boost::shared_ptr<NodeModule>(new SubpatchModule(canvas, patch));
	else
		ret = boost::shared_ptr<NodeModule>(new NodeModule(canvas, node));

	for (MetadataMap::const_iterator m = node->metadata().begin(); m != node->metadata().end(); ++m)
		ret->set_metadata(m->first, m->second);

	for (PortModelList::const_iterator p = node->ports().begin(); p != node->ports().end(); ++p) {
		ret->add_port(*p, false);
	}

	ret->resize();

	return ret;
}

	
void
NodeModule::control_change(uint32_t index, float control)
{
	if (_slv2_ui) {
		const LV2UI_Descriptor* const ui_descriptor = slv2_ui_instance_get_descriptor(_slv2_ui);
		LV2UI_Handle ui_handle = slv2_ui_instance_get_handle(_slv2_ui);
		ui_descriptor->port_event(ui_handle, index, 4, &control);
	}
}


void
NodeModule::embed_gui(bool embed)
{
	if (embed) {
			
		// FIXME: leaks?
				
		GtkWidget* c_widget = NULL;
		Gtk::Bin* container = NULL;

		if (!_gui_item) {
			cerr << "Embedding LV2 GUI" << endl;

			_slv2_ui = _node->plugin()->ui(App::instance().engine().get(), _node.get());
			if (_slv2_ui) {
				cerr << "Found UI" << endl;
				c_widget = (GtkWidget*)slv2_ui_instance_get_widget(_slv2_ui);
				_gui = Glib::wrap(c_widget);
				assert(_gui);
			
				//container = new Gtk::Alignment();  // transparent bg but uber slow
				container = new Gtk::EventBox();
				container->set_name("ingen_embedded_node_gui_container");
				container->set_border_width(2);
				container->add(*_gui);
				container->show_all();
				/*Gdk::Color color;
				color.set_red((_color & 0xFF000000) >> 24);
				color.set_green((_color & 0x00FF0000) >> 16);
				color.set_blue((_color & 0xFF000000) >> 8);
				container->modify_bg(Gtk::STATE_NORMAL,   color);
				container->modify_bg(Gtk::STATE_ACTIVE,   color);
				container->modify_bg(Gtk::STATE_PRELIGHT, color);
				container->modify_bg(Gtk::STATE_SELECTED, color);*/
			
				const double y = 4 + _canvas_title.property_text_height();
				_gui_item = new Gnome::Canvas::Widget(*this, 2.0, y, *container);
			}
		}

		if (_gui_item) {
			assert(_gui);
			cerr << "Created canvas item" << endl;
			_gui->show_all();
			_gui_item->show();

			Gtk::Requisition r = container->size_request();
			gui_size_request(&r);

			_gui_item->raise_to_top();

			container->signal_size_request().connect(
					sigc::mem_fun(this, &NodeModule::gui_size_request));
		
			for (PortModelList::const_iterator p = _node->ports().begin(); p != _node->ports().end(); ++p)
				if ((*p)->is_control() && (*p)->is_output())
					App::instance().engine()->enable_port_broadcasting((*p)->path());

		} else {
			cerr << "*** Failed to create canvas item" << endl;
		}

	} else {
		if (_gui_item) {
			delete _gui_item;
			_gui_item = NULL;
		}

		slv2_ui_instance_free(_slv2_ui);
		_slv2_ui = NULL;
		_gui = NULL;

		_ports_y_offset = 0;
		_width = 0; // resize() takes care of it..

		for (PortModelList::const_iterator p = _node->ports().begin(); p != _node->ports().end(); ++p)
			if ((*p)->is_control() && (*p)->is_output())
				App::instance().engine()->disable_port_broadcasting((*p)->path());
	}
			
	if (embed && _gui_item) {
		initialise_gui_values();
		set_base_color(0x212222FF);
	} else {
		set_default_base_color();
	}

	resize();
}
	

void
NodeModule::gui_size_request(Gtk::Requisition* r)
{
	if (r->width + 4 > _width)
		set_width(r->width + 4);

	_gui_item->property_width() = _width - 4;
	_gui_item->property_height() = r->height;
	_ports_y_offset = r->height + 2;

	resize();
}


void
NodeModule::rename()
{
	set_name(_node->path().name());
}


void
NodeModule::add_port(SharedPtr<PortModel> port, bool resize_to_fit)
{
	uint32_t index = _ports.size(); // FIXME: kluge, engine needs to tell us this
	
	Module::add_port(boost::shared_ptr<Port>(
			new Port(PtrCast<NodeModule>(shared_from_this()), port)));
		
	port->signal_control.connect(sigc::bind<0>(
			sigc::mem_fun(this, &NodeModule::control_change), index));

	if (resize_to_fit)
		resize();
}


void
NodeModule::remove_port(SharedPtr<PortModel> port)
{
	SharedPtr<FlowCanvas::Port> p = Module::remove_port(port->path().name());
	p.reset();
}


bool
NodeModule::popup_gui()
{
#ifdef HAVE_SLV2
	if (_node->plugin()->type() == PluginModel::LV2) {
		if (_slv2_ui) {
			cerr << "LV2 GUI already embedded" << endl;
			return false;
		}

		_slv2_ui = _node->plugin()->ui(App::instance().engine().get(), _node.get());
		if (_slv2_ui) {
			cerr << "Popping up LV2 GUI" << endl;

			// FIXME: leaks
			
			GtkWidget* c_widget = (GtkWidget*)slv2_ui_instance_get_widget(_slv2_ui);
			_gui = Glib::wrap(c_widget);
			
			Gtk::Window* win = new Gtk::Window();
			win->add(*_gui);
			_gui->show_all();
			initialise_gui_values();

			win->present();
			return true;
		} else {
			cerr << "No LV2 GUI" << endl;
		}
	}
#endif
	return false;
}


void
NodeModule::initialise_gui_values()
{
	uint32_t index=0;
	for (PortModelList::const_iterator p = _node->ports().begin(); p != _node->ports().end(); ++p) {
		if ((*p)->is_control())
			control_change(index, (*p)->value());
		++index;
	}
}


void
NodeModule::show_control_window()
{
	App::instance().window_factory()->present_controls(_node);
}
	

void
NodeModule::on_double_click(GdkEventButton* ev)
{
	if ( ! popup_gui() )
		show_control_window();
}


void
NodeModule::store_location()
{
	const float x = static_cast<float>(property_x());
	const float y = static_cast<float>(property_y());
	
	const Atom& existing_x = _node->get_metadata("ingenuity:canvas-x");
	const Atom& existing_y = _node->get_metadata("ingenuity:canvas-y");
	
	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		App::instance().engine()->set_metadata(_node->path(), "ingenuity:canvas-x", Atom(x));
		App::instance().engine()->set_metadata(_node->path(), "ingenuity:canvas-y", Atom(y));
	}
}


void
NodeModule::set_metadata(const string& key, const Atom& value)
{
	if (key == "ingenuity:canvas-x" && value.type() == Atom::FLOAT)
		move_to(value.get_float(), property_y());
	else if (key == "ingenuity:canvas-y" && value.type() == Atom::FLOAT)
		move_to(property_x(), value.get_float());
}


} // namespace GUI
} // namespace Ingen
