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
#include "NodeModel.h"
#include "NodeModule.h"
#include "SubpatchModule.h"
#include "GladeFactory.h"
#include "WindowFactory.h"

namespace Ingenuity {


PatchCanvas::PatchCanvas(CountedPtr<PatchModel> patch, int width, int height)
: FlowCanvas(width, height),
  m_patch(patch),
  m_last_click_x(0),
  m_last_click_y(0)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeFactory::new_glade_reference();
	xml->get_widget("canvas_menu", m_menu);
	
	xml->get_widget("canvas_menu_add_audio_input", m_menu_add_audio_input);
	xml->get_widget("canvas_menu_add_audio_output", m_menu_add_audio_output);
	xml->get_widget("canvas_menu_add_control_input", m_menu_add_control_input);
	xml->get_widget("canvas_menu_add_control_output", m_menu_add_control_output);
	xml->get_widget("canvas_menu_add_midi_input", m_menu_add_midi_input);
	xml->get_widget("canvas_menu_add_midi_output", m_menu_add_midi_output);
	xml->get_widget("canvas_menu_load_plugin", m_menu_load_plugin);
	xml->get_widget("canvas_menu_load_patch", m_menu_load_patch);
	xml->get_widget("canvas_menu_new_patch", m_menu_new_patch);
	
	build_canvas();

	// Add port menu items
	m_menu_add_audio_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_input", "AUDIO", false));
	m_menu_add_audio_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"audio_output", "AUDIO", true));
	m_menu_add_control_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_input", "CONTROL", false));
	m_menu_add_control_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"control_output", "CONTROL", true));
	m_menu_add_midi_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"midi_input", "MIDI", false));
	m_menu_add_midi_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &PatchCanvas::menu_add_port),
			"midi_output", "MIDI", true));

	// Connect to model signals to track state
	m_patch->new_node_sig.connect(sigc::mem_fun(this, &PatchCanvas::add_node));
	m_patch->removed_node_sig.connect(sigc::mem_fun(this, &PatchCanvas::remove_node));
	m_patch->new_port_sig.connect(sigc::mem_fun(this, &PatchCanvas::add_port));
	m_patch->removed_port_sig.connect(sigc::mem_fun(this, &PatchCanvas::remove_port));
	m_patch->new_connection_sig.connect(sigc::mem_fun(this, &PatchCanvas::connection));
	m_patch->removed_connection_sig.connect(sigc::mem_fun(this, &PatchCanvas::disconnection));
	
	// Connect widget signals to do things
	m_menu_load_plugin->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_plugin));
	m_menu_load_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_load_patch));
	m_menu_new_patch->signal_activate().connect(sigc::mem_fun(this, &PatchCanvas::menu_new_patch));
}


void
PatchCanvas::build_canvas() {
	
	// Create modules for nodes
	for (NodeModelMap::const_iterator i = m_patch->nodes().begin();
			i != m_patch->nodes().end(); ++i) {
		add_node((*i).second);
	}

	// Create pseudo modules for ports (ports on this canvas, not on our module)
	for (PortModelList::const_iterator i = m_patch->ports().begin();
			i != m_patch->ports().end(); ++i) {
		manage(new PatchPortModule(this, *i));
	}

	// Create connections
	for (list<CountedPtr<ConnectionModel> >::const_iterator i = m_patch->connections().begin();
			i != m_patch->connections().end(); ++i) {
		connection(*i);
	}
}


void
PatchCanvas::add_node(CountedPtr<NodeModel> nm)
{
	CountedPtr<PatchModel> pm = PtrCast<PatchModel>(nm);
	if (pm)
		manage(new SubpatchModule(this, pm));
	else
		manage(new NodeModule(this, nm));
}


void
PatchCanvas::remove_node(CountedPtr<NodeModel> nm)
{
	LibFlowCanvas::Module* module = get_module(nm->path().name());
	delete module;
}


void
PatchCanvas::add_port(CountedPtr<PortModel> pm)
{
	manage(new PatchPortModule(this, pm));
}


void
PatchCanvas::remove_port(CountedPtr<PortModel> pm)
{
	cerr << "FIXME: PORT REMOVE" << endl;
	//LibFlowCanvas::Module* module = get_module(pm->path().name());
	//delete module;
}


void
PatchCanvas::connection(CountedPtr<ConnectionModel> cm)
{
	// Deal with port "anonymous nodes" for this patch's own ports...
	const Path& src_parent_path = cm->src_port_path().parent();
	const Path& dst_parent_path = cm->dst_port_path().parent();

	const string& src_parent_name =
		(src_parent_path == m_patch->path()) ? "" : src_parent_path.name();
	const string& dst_parent_name =
		(dst_parent_path == m_patch->path()) ? "" : dst_parent_path.name();

	LibFlowCanvas::Port* src_port = get_port(src_parent_name, cm->src_port_path().name());
	LibFlowCanvas::Port* dst_port = get_port(dst_parent_name, cm->dst_port_path().name());
	assert(src_port && dst_port);

	add_connection(src_port, dst_port);
}


void
PatchCanvas::disconnection(const Path& src_port_path, const Path& dst_port_path)
{
	const string& src_node_name = src_port_path.parent().name();
	const string& src_port_name = src_port_path.name();
	const string& dst_node_name = dst_port_path.parent().name();
	const string& dst_port_name = dst_port_path.name();

	LibFlowCanvas::Port* src_port = get_port(src_node_name, src_port_name);
	LibFlowCanvas::Port* dst_port = get_port(dst_node_name, dst_port_name);

	if (src_port && dst_port) {
		remove_connection(src_port, dst_port);
	}

	//patch_model()->remove_connection(src_port_path, dst_port_path);

	cerr << "FIXME: disconnection\n";
	/*
	// Enable control slider in destination node control window
	PortController* p = (PortController)Store::instance().port(dst_port_path)->controller();
	assert(p);

	if (p->control_panel())
	p->control_panel()->enable_port(p->path());
	*/
}


void
PatchCanvas::connect(const LibFlowCanvas::Port* src_port, const LibFlowCanvas::Port* dst_port)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
	
	const Ingenuity::Port* const src = dynamic_cast<const Ingenuity::Port* const>(src_port);
	const Ingenuity::Port* const dst = dynamic_cast<const Ingenuity::Port* const>(dst_port);
	assert(src && dst);

	// Midi binding/learn shortcut
	if (src->model()->type() == PortModel::MIDI &&
			dst->model()->type() == PortModel::CONTROL)
	{
		cerr << "FIXME: MIDI binding" << endl;
#if 0
		CountedPtr<PluginModel> pm(new PluginModel(PluginModel::Internal, "", "midi_control_in", ""));
		CountedPtr<NodeModel> nm(new NodeModel(pm, m_patch->path().base()
			+ src->name() + "-" + dst->name(), false));
		nm->set_metadata("module-x", Atom((float)
			(dst->module()->property_x() - dst->module()->width() - 20)));
		nm->set_metadata("module-y", Atom((float)
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
PatchCanvas::disconnect(const LibFlowCanvas::Port* src_port, const LibFlowCanvas::Port* dst_port)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
	
	App::instance().engine()->disconnect(((Ingenuity::Port*)src_port)->model()->path(),
	                       ((Ingenuity::Port*)dst_port)->model()->path());
}


bool
PatchCanvas::canvas_event(GdkEvent* event)
{
	assert(event != NULL);
	
	switch (event->type) {

	case GDK_BUTTON_PRESS:
		if (event->button.button == 3) {
			m_last_click_x = (int)event->button.x;
			m_last_click_y = (int)event->button.y;
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
PatchCanvas::destroy_selected()
{
	for (list<Module*>::iterator m = m_selected_modules.begin(); m != m_selected_modules.end(); ++m)
		App::instance().engine()->destroy(((NodeModule*)(*m))->node()->path());
}


string
PatchCanvas::generate_port_name(const string& base) {
	string name = base;

	char num_buf[5];
	for (uint i=1; i < 9999; ++i) {
		snprintf(num_buf, 5, "%u", i);
		name = base + "_";
		name += num_buf;
		if (!m_patch->get_port(name))
			break;
	}

	assert(Path::is_valid(string("/") + name));

	return name;
}


void
PatchCanvas::menu_add_port(const string& name, const string& type, bool is_output)
{
	const Path& path = m_patch->path().base() + generate_port_name(name);
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
	
	result["module-x"] = Atom((float)m_last_click_x);
	result["module-y"] = Atom((float)m_last_click_y);
	
	return result;
}

void
PatchCanvas::menu_load_plugin()
{
	App::instance().window_factory()->present_load_plugin(m_patch, get_initial_data());
}


void
PatchCanvas::menu_load_patch()
{
	App::instance().window_factory()->present_load_subpatch(m_patch, get_initial_data());
}


void
PatchCanvas::menu_new_patch()
{
	App::instance().window_factory()->present_new_subpatch(m_patch, get_initial_data());
}


} // namespace Ingenuity
