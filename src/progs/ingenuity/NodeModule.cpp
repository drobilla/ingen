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

#include "NodeModule.h"
#include <cassert>
#include "raul/Atom.h"
#include "App.h"
#include "ModelEngineInterface.h"
#include "PatchCanvas.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "Port.h"
#include "GladeFactory.h"
#include "RenameWindow.h"
#include "PatchWindow.h"
#include "WindowFactory.h"
#include "SubpatchModule.h"
#include "NodeControlWindow.h"

namespace Ingenuity {


NodeModule::NodeModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<NodeModel> node)
: LibFlowCanvas::Module(canvas, node->path().name()),
  m_node(node),
  m_menu(node)
{
	assert(m_node);

	if (node->polyphonic()) {
		set_border_width(2.0);
	}

	node->new_port_sig.connect(sigc::bind(sigc::mem_fun(this, &NodeModule::add_port), true));
	node->removed_port_sig.connect(sigc::mem_fun(this, &NodeModule::remove_port));
	node->metadata_update_sig.connect(sigc::mem_fun(this, &NodeModule::metadata_update));
}


NodeModule::~NodeModule()
{
	NodeControlWindow* win = App::instance().window_factory()->control_window(m_node);
	
	if (win) {
		// Should remove from window factory via signal
		delete win;
	}
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
		ret->metadata_update(m->first, m->second);

	for (PortModelList::const_iterator p = node->ports().begin(); p != node->ports().end(); ++p)
		ret->add_port(*p, false);

	ret->resize();

	return ret;
}


void
NodeModule::add_port(SharedPtr<PortModel> port, bool resize_to_fit)
{
	Module::add_port(boost::shared_ptr<Port>(new Port(shared_from_this(), port)));
	if (resize_to_fit)
		resize();
}


void
NodeModule::remove_port(SharedPtr<PortModel> port)
{
	SharedPtr<LibFlowCanvas::Port> p = Module::remove_port(port->path().name());
	p.reset();
}


void
NodeModule::show_control_window()
{
	App::instance().window_factory()->present_controls(m_node);
}


void
NodeModule::store_location()
{
	const float x = static_cast<float>(property_x());
	const float y = static_cast<float>(property_y());
	
	const Atom& existing_x = m_node->get_metadata("ingenuity:canvas-x");
	const Atom& existing_y = m_node->get_metadata("ingenuity:canvas-y");
	
	if (existing_x.type() != Atom::FLOAT || existing_y.type() != Atom::FLOAT
			|| existing_x.get_float() != x || existing_y.get_float() != y) {
		App::instance().engine()->set_metadata(m_node->path(), "ingenuity:canvas-x", Atom(x));
		App::instance().engine()->set_metadata(m_node->path(), "ingenuity:canvas-y", Atom(y));
	}
}


void
NodeModule::on_right_click(GdkEventButton* event)
{
	m_menu.popup(event->button, event->time);
}


void
NodeModule::metadata_update(const string& key, const Atom& value)
{
	if (key == "ingenuity:canvas-x" && value.type() == Atom::FLOAT)
		move_to(value.get_float(), property_y());
	else if (key == "ingenuity:canvas-y" && value.type() == Atom::FLOAT)
		move_to(property_x(), value.get_float());
}


} // namespace Ingenuity
