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

#include "PatchCanvas.h"
#include <cassert>
#include <flowcanvas/FlowCanvas.h>
#include "App.h"
#include "ModelEngineInterface.h"
#include "PatchModel.h"
#include "PatchWindow.h"
#include "PatchPortModule.h"
#include "LoadPluginWindow.h"
#include "LoadSubpatchWindow.h"
#include "NewSubpatchWindow.h"
#include "Port.h"
#include "Connection.h"
#include "NodeModel.h"
#include "NodeModule.h"
#include "SubpatchModule.h"
#include "GladeFactory.h"
#include "WindowFactory.h"
#include "Serializer.h"
using Ingen::Client::Serializer;

namespace Ingenuity {


PatchCanvas::PatchCanvas(SharedPtr<PatchModel> patch, int width, int height)
: FlowCanvas(width, height),
  _patch(patch),
  _last_click_x(0),
  _last_click_y(0)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget("canvas_menu", _menu);
	
	xml->get_widget("canvas_menu_add_audio_input", _menu_add_audio_input);
	xml->get_widget("canvas_menu_add_audio_output", _menu_add_audio_output);
	xml->get_widget("canvas_menu_add_control_input", _menu_add_control_input);
	xml->get_widget("canvas_menu_add_control_output", _menu_add_control_output);
	xml->get_widget("canvas_menu_add_midi_input", _menu_add_midi_input);
	xml->get_widget("canvas_menu_add_midi_output", _menu_add_midi_output);
	xml->get_widget("canvas_menu_load_plugin", _menu_load_plugin);
	xml->get_widget("canvas_menu_load_patch", _menu_load_patch);
	xml->get_widget("canvas_menu_new_patch", _menu_new_patch);
	
	// Add port menu items
	_menu_add_audio_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_input", "ingen:audio", false));
	_menu_add_audio_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_output", "ingen:audio", true));
	_menu_add_control_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_input", "ingen:control", false));
	_menu_add_control_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_output", "ingen:control", true));
	_menu_add_midi_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"midi_input", "ingen:midi", false));
	_menu_add_midi_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"midi_output", "ingen:midi", true));

	// Connect to model signals to track state
	_patch->new_node_sig.connect(sigc::mem_fun(this, &PatchCanvas::add_node));
	_patch->removed_node_sig.connect(sigc::mem_fun(this, &PatchCanvas::remove_node));
	_patch->new_port_sig.connect(sigc::mem_fun(this, &PatchCanvas::add_port));
	_patch->removed_port_sig.connect(sigc::mem_fun(this, &PatchCanvas::remove_port));
	_patch->new_connection_sig.connect(sigc::mem_fun(this, &PatchCanvas::connection));
	_patch->removed_connection_sig.connect(sigc::mem_fun(this, &PatchCanvas::disconnection));
	
	// Connect widget signals to do things
	_menu_load_plugin->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_plugin));
	_menu_load_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_patch));
	_menu_new_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_new_patch));
}


void
PatchCanvas::build()
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());
	
	// Create modules for nodes
	for (NodeModelMap::const_iterator i = _patch->nodes().begin();
			i != _patch->nodes().end(); ++i) {
		add_node((*i).second);
	}

	// Create pseudo modules for ports (ports on this canvas, not on our module)
	for (PortModelList::const_iterator i = _patch->ports().begin();
			i != _patch->ports().end(); ++i) {
		add_port(*i);
	}

	// Create connections
	for (list<SharedPtr<ConnectionModel> >::const_iterator i = _patch->connections().begin();
			i != _patch->connections().end(); ++i) {
		connection(*i);
	}
}


void
PatchCanvas::add_node(SharedPtr<NodeModel> nm)
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());

	SharedPtr<PatchModel> pm = PtrCast<PatchModel>(nm);
	SharedPtr<NodeModule> module;
	if (pm)
		module = SubpatchModule::create(shared_this, pm);
	else
		module = NodeModule::create(shared_this, nm);

	add_item(module);
	module->show();
	_views.insert(std::make_pair(nm, module));
}


void
PatchCanvas::remove_node(SharedPtr<NodeModel> nm)
{
	Views::iterator i = _views.find(nm);

	if (i != _views.end()) {
		remove_item(i->second);
		_views.erase(i);
	}
}


void
PatchCanvas::add_port(SharedPtr<PortModel> pm)
{
	boost::shared_ptr<PatchCanvas> shared_this =
		boost::dynamic_pointer_cast<PatchCanvas>(shared_from_this());

	SharedPtr<PatchPortModule> view = PatchPortModule::create(shared_this, pm);
	_views.insert(std::make_pair(pm, view));
	add_item(view);
	view->show();
}


void
PatchCanvas::remove_port(SharedPtr<PortModel> pm)
{
	Views::iterator i = _views.find(pm);

	if (i != _views.end()) {
		remove_item(i->second);
		_views.erase(i);
	}
}


SharedPtr<LibFlowCanvas::Port>
PatchCanvas::get_port_view(SharedPtr<PortModel> port)
{
	SharedPtr<LibFlowCanvas::Port> ret;
	SharedPtr<LibFlowCanvas::Module> module = _views[port];
	
	// Port on this patch
	if (module) {
		ret = (PtrCast<PatchPortModule>(module))
			? *(PtrCast<PatchPortModule>(module)->ports().begin())
			: PtrCast<LibFlowCanvas::Port>(module);
	} else {
		module = PtrCast<NodeModule>(_views[port->parent()]);
		if (module)
			ret = module->get_port(port->path().name());
	}
	
	return ret;
}


void
PatchCanvas::connection(SharedPtr<ConnectionModel> cm)
{
	const SharedPtr<LibFlowCanvas::Port> src = get_port_view(cm->src_port());
	const SharedPtr<LibFlowCanvas::Port> dst = get_port_view(cm->dst_port());

	if (src && dst)
		add_connection(boost::shared_ptr<Connection>(new Connection(shared_from_this(),
				cm, src, dst, src->color() + 0x22222200)));
	else
		cerr << "[PatchCanvas] ERROR: Unable to find ports to connect "
			<< cm->src_port_path() << " -> " << cm->dst_port_path() << endl;
}


void
PatchCanvas::disconnection(SharedPtr<ConnectionModel> cm)
{
	const SharedPtr<LibFlowCanvas::Port> src = get_port_view(cm->src_port());
	const SharedPtr<LibFlowCanvas::Port> dst = get_port_view(cm->dst_port());
		
	if (src && dst)
		remove_connection(src, dst);
	else
		cerr << "[PatchCanvas] ERROR: Unable to find ports to disconnect "
			<< cm->src_port_path() << " -> " << cm->dst_port_path() << endl;
}


void
PatchCanvas::connect(boost::shared_ptr<LibFlowCanvas::Connectable> src_port,
                     boost::shared_ptr<LibFlowCanvas::Connectable> dst_port)
{
	const boost::shared_ptr<Ingenuity::Port> src
		= boost::dynamic_pointer_cast<Ingenuity::Port>(src_port);

	const boost::shared_ptr<Ingenuity::Port> dst
		= boost::dynamic_pointer_cast<Ingenuity::Port>(dst_port);
	
	if (!src || !dst)
		return;

	// Midi binding/learn shortcut
	if (src->model()->is_midi() && dst->model()->is_control())
	{
		cerr << "FIXME: MIDI binding" << endl;
#if 0
		SharedPtr<PluginModel> pm(new PluginModel(PluginModel::Internal, "", "midi_control_in", ""));
		SharedPtr<NodeModel> nm(new NodeModel(pm, _patch->path().base()
			+ src->name() + "-" + dst->name(), false));
		nm->set_metadata("canvas-x", Atom((float)
			(dst->module()->property_x() - dst->module()->width() - 20)));
		nm->set_metadata("canvas-y", Atom((float)
			(dst->module()->property_y())));
		App::instance().engine()->create_node_from_model(nm.get());
		App::instance().engine()->connect(src->model()->path(), nm->path() + "/MIDI_In");
		App::instance().engine()->connect(nm->path() + "/Out_(CR)", dst->model()->path());
		App::instance().engine()->midi_learn(nm->path());
		
		// Set control node range to port's user range
		
		App::instance().engine()->set_port_value_queued(nm->path().base() + "Min",
			dst->model()->get_metadata("user-min").get_float());
		App::instance().engine()->set_port_value_queued(nm->path().base() + "Max",
			dst->model()->get_metadata("user-max").get_float());
#endif
	} else {
		App::instance().engine()->connect(src->model()->path(), dst->model()->path());
	}
}


void
PatchCanvas::disconnect(boost::shared_ptr<LibFlowCanvas::Connectable> src_port,
                        boost::shared_ptr<LibFlowCanvas::Connectable> dst_port)
{
	const boost::shared_ptr<Ingenuity::Port> src
		= boost::dynamic_pointer_cast<Ingenuity::Port>(src_port);

	const boost::shared_ptr<Ingenuity::Port> dst
		= boost::dynamic_pointer_cast<Ingenuity::Port>(dst_port);
	
	App::instance().engine()->disconnect(src->model()->path(),
	                                     dst->model()->path());
}


bool
PatchCanvas::canvas_event(GdkEvent* event)
{
	assert(event);
	
	switch (event->type) {

	case GDK_BUTTON_PRESS:
		if (event->button.button == 3) {
			_last_click_x = (int)event->button.x;
			_last_click_y = (int)event->button.y;
			show_menu(event);
		}
		break;
	
	/*case GDK_KEY_PRESS:
		if (event->key.keyval == GDK_Delete)
			destroy_selected();
		break;
	*/
		
	default:
		break;
	}

	return FlowCanvas::canvas_event(event);
}


void
PatchCanvas::destroy_selection()
{
	for (list<boost::shared_ptr<Item> >::iterator m = _selected_items.begin(); m != _selected_items.end(); ++m) {
		boost::shared_ptr<NodeModule> module = boost::dynamic_pointer_cast<NodeModule>(*m);
		if (module) {
			App::instance().engine()->destroy(module->node()->path());
		} else {
			boost::shared_ptr<PatchPortModule> port_module = boost::dynamic_pointer_cast<PatchPortModule>(*m);
			if (port_module)
				App::instance().engine()->destroy(port_module->port()->path());
		}
	}

}


void
PatchCanvas::copy_selection()
{
	Serializer serializer;
	serializer.start_to_string();

	for (list<boost::shared_ptr<Item> >::iterator m = _selected_items.begin(); m != _selected_items.end(); ++m) {
		boost::shared_ptr<NodeModule> module = boost::dynamic_pointer_cast<NodeModule>(*m);
		if (module) {
			serializer.serialize(module->node());
		} else {
			boost::shared_ptr<PatchPortModule> port_module = boost::dynamic_pointer_cast<PatchPortModule>(*m);
			if (port_module)
				serializer.serialize(port_module->port());
		}
	}
	
	for (list<boost::shared_ptr<LibFlowCanvas::Connection> >::iterator c = _selected_connections.begin();
			c != _selected_connections.end(); ++c) {
		boost::shared_ptr<Connection> connection = boost::dynamic_pointer_cast<Connection>(*c);
		if (connection)
			serializer.serialize_connection(connection->model());
	}
	
	string result = serializer.finish();
	
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	clipboard->set_text(result);
}


string
PatchCanvas::generate_port_name(const string& base) {
	string name = base;

	char num_buf[5];
	for (uint i=1; i < 9999; ++i) {
		snprintf(num_buf, 5, "%u", i);
		name = base + "_";
		name += num_buf;
		if (!_patch->get_port(name))
			break;
	}

	assert(Path::is_valid(string("/") + name));

	return name;
}


void
PatchCanvas::menu_add_port(const string& name, const string& type, bool is_output)
{
	const Path& path = _patch->path().base() + generate_port_name(name);
	App::instance().engine()->create_port_with_data(path, type, is_output, get_initial_data());
}


/** Try to guess a suitable location for a new module.
 */
void
PatchCanvas::get_new_module_location(double& x, double& y)
{
	int scroll_x;
	int scroll_y;
	get_scroll_offsets(scroll_x, scroll_y);
	x = scroll_x + 20;
	y = scroll_y + 20;
}


MetadataMap
PatchCanvas::get_initial_data()
{
	MetadataMap result;
	
	result["ingenuity:canvas-x"] = Atom((float)_last_click_x);
	result["ingenuity:canvas-y"] = Atom((float)_last_click_y);
	
	return result;
}

void
PatchCanvas::menu_load_plugin()
{
	App::instance().window_factory()->present_load_plugin(_patch, get_initial_data());
}


void
PatchCanvas::menu_load_patch()
{
	App::instance().window_factory()->present_load_subpatch(_patch, get_initial_data());
}


void
PatchCanvas::menu_new_patch()
{
	App::instance().window_factory()->present_new_subpatch(_patch, get_initial_data());
}


} // namespace Ingenuity
