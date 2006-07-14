/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "OmFlowCanvas.h"
#include <cassert>
#include <flowcanvas/FlowCanvas.h>
#include "Controller.h"
#include "PatchController.h"
#include "PatchModel.h"
#include "PatchWindow.h"
#include "LoadPluginWindow.h"
#include "LoadSubpatchWindow.h"
#include "NewSubpatchWindow.h"
#include "OmPort.h"
#include "NodeModel.h"
#include "OmModule.h"
#include "GladeFactory.h"

namespace OmGtk {


OmFlowCanvas::OmFlowCanvas(PatchController* controller, int width, int height)
: FlowCanvas(width, height),
  m_patch_controller(controller),
  m_last_click_x(0),
  m_last_click_y(0)
{
	assert(controller != NULL);
	
	/*Gtk::Menu::MenuList& items = m_menu.items();
	items.push_back(Gtk::Menu_Helpers::MenuElem("Load Plugin...",
		sigc::mem_fun(this, &OmFlowCanvas::menu_load_plugin)));
	items.push_back(Gtk::Menu_Helpers::MenuElem("Load Subpatch...",
		sigc::mem_fun(this, &OmFlowCanvas::menu_load_subpatch)));
	items.push_back(Gtk::Menu_Helpers::MenuElem("New Subpatch...",
		sigc::mem_fun(this, &OmFlowCanvas::menu_create_subpatch)));*/

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
	
	// Add port menu items
	m_menu_add_audio_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &OmFlowCanvas::menu_add_port),
			"audio_input", "AUDIO", false));
	m_menu_add_audio_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &OmFlowCanvas::menu_add_port),
			"audio_output", "AUDIO", true));
	m_menu_add_control_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &OmFlowCanvas::menu_add_port),
			"control_input", "CONTROL", false));
	m_menu_add_control_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &OmFlowCanvas::menu_add_port),
			"control_output", "CONTROL", true));
	m_menu_add_midi_input->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &OmFlowCanvas::menu_add_port),
			"midi_input", "MIDI", false));
	m_menu_add_midi_output->signal_activate().connect(
		sigc::bind(sigc::mem_fun(this, &OmFlowCanvas::menu_add_port),
			"midi_output", "MIDI", true));

	m_menu_load_plugin->signal_activate().connect(sigc::mem_fun(this, &OmFlowCanvas::menu_load_plugin));
	m_menu_load_patch->signal_activate().connect(sigc::mem_fun(this, &OmFlowCanvas::menu_load_patch));
	m_menu_new_patch->signal_activate().connect(sigc::mem_fun(this, &OmFlowCanvas::menu_new_patch));
}


void
OmFlowCanvas::connect(const Port* src_port, const Port* dst_port)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
	
	const OmPort* const src = static_cast<const OmPort* const>(src_port);
	const OmPort* const dst = static_cast<const OmPort* const>(dst_port);
	
	// Midi binding/learn shortcut
	if (src->model()->type() == PortModel::MIDI &&
			dst->model()->type() == PortModel::CONTROL)
	{
		// FIXME: leaks?
		PluginModel* pm = new PluginModel(PluginModel::Internal, "", "midi_control_in", "");
		NodeModel* nm = new NodeModel(pm, m_patch_controller->model()->base_path()
			+ src->name() + "-" + dst->name());
		nm->x(dst->module()->property_x() - dst->module()->width() - 20);
		nm->y(dst->module()->property_y());
		Controller::instance().create_node_from_model(nm);
		Controller::instance().connect(src->model()->path(), nm->path() + "/MIDI_In");
		Controller::instance().connect(nm->path() + "/Out_(CR)", dst->model()->path());
		Controller::instance().midi_learn(nm->path());
		
		// Set control node range to port's user range
		
		Controller::instance().set_port_value_queued(nm->path().base_path() + "Min",
			atof(dst->model()->get_metadata("user-min").c_str()));
		Controller::instance().set_port_value_queued(nm->path().base_path() + "Max",
			atof(dst->model()->get_metadata("user-max").c_str()));
	} else {
		Controller::instance().connect(src->model()->path(),
		                    dst->model()->path());
	}
}


void
OmFlowCanvas::disconnect(const Port* src_port, const Port* dst_port)
{
	assert(src_port != NULL);
	assert(dst_port != NULL);
	
	Controller::instance().disconnect(((OmPort*)src_port)->model()->path(),
	                       ((OmPort*)dst_port)->model()->path());
}


bool
OmFlowCanvas::canvas_event(GdkEvent* event)
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
OmFlowCanvas::destroy_selected()
{
	for (list<Module*>::iterator m = m_selected_modules.begin(); m != m_selected_modules.end(); ++m)
		Controller::instance().destroy(((OmModule*)(*m))->node()->path());
}


string
OmFlowCanvas::generate_port_name(const string& base) {
	string name = base;

	char num_buf[5];
	for (uint i=1; i < 9999; ++i) {
		snprintf(num_buf, 5, "%d", i);
		name = base + "_";
		name += num_buf;
		if (!m_patch_controller->patch_model()->get_port(name))
			break;
	}

	return name;
}


void
OmFlowCanvas::menu_add_port(const string& name, const string& type, bool is_output)
{
	const Path& path = m_patch_controller->path().base_path() + generate_port_name(name);
	Controller::instance().create_port(path, type, is_output);
	
	char temp_buf[16];
	snprintf(temp_buf, 16, "%d", m_last_click_x);
	Controller::instance().set_metadata(path, "module-x", temp_buf);
	snprintf(temp_buf, 16, "%d", m_last_click_y);
	Controller::instance().set_metadata(path, "module-y", temp_buf);
}


/*
void
OmFlowCanvas::menu_add_audio_input()
{
	string name = "audio_in";
	Controller::instance().create_port(m_patch_controller->path().base_path() + name, "AUDIO", false);
}


void
OmFlowCanvas::menu_add_audio_output()
{
	string name = "audio_out";
	Controller::instance().create_port(m_patch_controller->path().base_path() + name, "AUDIO", true);
}


void
OmFlowCanvas::menu_add_control_input()
{
	string name = "control_in";
	Controller::instance().create_port(m_patch_controller->path().base_path() + name, "CONTROL", false);
}


void
OmFlowCanvas::menu_add_control_output()
{
	string name = "control_out";
	Controller::instance().create_port(m_patch_controller->path().base_path() + name, "CONTROL", true);
}


void
OmFlowCanvas::menu_add_midi_input()
{
	string name = "midi_in";
	Controller::instance().create_port(m_patch_controller->path().base_path() + name, "MIDI", false);
}


void
OmFlowCanvas::menu_add_midi_output()
{
	string name = "midi_out";
	Controller::instance().create_port(m_patch_controller->path().base_path() + name, "MIDI", true);
}
*/

void
OmFlowCanvas::menu_load_plugin()
{
	m_patch_controller->window()->load_plugin_window()->set_next_module_location(
		m_last_click_x, m_last_click_y);
	m_patch_controller->window()->load_plugin_window()->show();
}


void
OmFlowCanvas::menu_load_patch()
{
	m_patch_controller->window()->load_subpatch_window()->set_next_module_location(
		m_last_click_x, m_last_click_y);
	m_patch_controller->window()->load_subpatch_window()->show();
}


void
OmFlowCanvas::menu_new_patch()
{
	m_patch_controller->window()->new_subpatch_window()->set_next_module_location(
		m_last_click_x, m_last_click_y);
	m_patch_controller->window()->new_subpatch_window()->show();
}


} // namespace OmGtk
