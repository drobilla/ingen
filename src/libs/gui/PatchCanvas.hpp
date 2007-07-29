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

#ifndef PATCHCANVAS_H
#define PATCHCANVAS_H

#include CONFIG_H_PATH

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Module.hpp>
#include <raul/SharedPtr.hpp>
#include <raul/Path.hpp>
#include "client/ConnectionModel.hpp"
#include "client/PatchModel.hpp"
#include "NodeModule.hpp"

using std::string;
using namespace FlowCanvas;

using FlowCanvas::Port;
using Ingen::Client::ConnectionModel;
using Ingen::Client::PatchModel;
using Ingen::Client::NodeModel;
using Ingen::Client::PortModel;
using Ingen::Client::MetadataMap;

namespace Ingen {
namespace GUI {
	
class NodeModule;


/** Patch canvas widget.
 *
 * \ingroup GUI
 */
class PatchCanvas : public FlowCanvas::Canvas
{
public:
	PatchCanvas(SharedPtr<PatchModel> patch, int width, int height);
	
	virtual ~PatchCanvas() {}

	/*boost::shared_ptr<NodeModule> find_module(const string& name) {
		return boost::dynamic_pointer_cast<NodeModule>(
		Canvas::get_item(name));
	}*/
	
	void build();
	void arrange();

	void add_node(SharedPtr<NodeModel> nm);
	void remove_node(SharedPtr<NodeModel> nm);
	void add_port(SharedPtr<PortModel> pm);
	void remove_port(SharedPtr<PortModel> pm);
	void connection(SharedPtr<ConnectionModel> cm);
	void disconnection(SharedPtr<ConnectionModel> cm);

	void get_new_module_location(double& x, double& y);

	void destroy_selection();
	void copy_selection();

	void show_menu(GdkEvent* event)
	{ _menu->popup(event->button.button, event->button.time); }
	
private:
	enum ControlType { NUMBER, BUTTON };
	void menu_add_control(ControlType type);
	
	string generate_port_name(const string& base);
	void menu_add_port(const string& name, const string& type, bool is_output);
	
	void menu_load_plugin();
	void menu_new_patch();
	void menu_load_patch();
	void load_plugin(SharedPtr<PluginModel> plugin);

#ifdef HAVE_SLV2
	void build_plugin_menu();
	size_t build_plugin_class_menu(Gtk::Menu* menu,
		SLV2PluginClass plugin_class, SLV2PluginClasses classes);
#endif
	
	MetadataMap get_initial_data();

	bool canvas_event(GdkEvent* event);
	
	SharedPtr<FlowCanvas::Port> get_port_view(SharedPtr<PortModel> port);

	void connect(boost::shared_ptr<FlowCanvas::Connectable> src,
	             boost::shared_ptr<FlowCanvas::Connectable> dst);

	void disconnect(boost::shared_ptr<FlowCanvas::Connectable> src,
	                boost::shared_ptr<FlowCanvas::Connectable> dst);

	SharedPtr<PatchModel> _patch;

	typedef std::map<SharedPtr<ObjectModel>, SharedPtr<FlowCanvas::Module> > Views;
	Views _views;

	int _last_click_x;
	int _last_click_y;
	
	Gtk::Menu*      _menu;
	Gtk::MenuItem*  _menu_add_number_control;
	Gtk::MenuItem*  _menu_add_button_control;
	Gtk::MenuItem*  _menu_add_audio_input;
	Gtk::MenuItem*  _menu_add_audio_output;
	Gtk::MenuItem*  _menu_add_control_input;
	Gtk::MenuItem*  _menu_add_control_output;
	Gtk::MenuItem*  _menu_add_midi_input;
	Gtk::MenuItem*  _menu_add_midi_output;
	Gtk::MenuItem*  _menu_add_osc_input;
	Gtk::MenuItem*  _menu_add_osc_output;
	Gtk::MenuItem*  _menu_load_plugin;
	Gtk::MenuItem*  _menu_load_patch;
	Gtk::MenuItem*  _menu_new_patch;
};


} // namespace GUI
} // namespace Ingen

#endif // PATCHCANVAS_H
