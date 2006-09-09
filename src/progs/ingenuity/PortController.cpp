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

#include "PortController.h"
#include "OmFlowCanvas.h"
#include "OmModule.h"
#include "PortModel.h"
#include "PatchModel.h"
#include "OmPort.h"
#include "OmPatchPort.h"
#include "Store.h"

namespace Ingenuity {


PortController::PortController(CountedPtr<PortModel> model)
: GtkObjectController(model),
  m_patch_port(NULL),
  m_module(NULL),
  m_port(NULL)
{
	assert(model);
	assert(model->parent());
	assert(model->controller() == NULL);

	model->set_controller(this);
}


void
PortController::destroy()
{
	assert(m_model->parent());
	NodeController* parent = (NodeController*)m_model->parent()->controller();
	assert(parent != NULL);

	parent->remove_port(path(), false);
}


void
PortController::create_module(OmFlowCanvas* canvas)
{
	//cerr << "Creating port module " << m_model->path() << endl;

	const string x_str = m_model->get_metadata("module-x");
	const string y_str = m_model->get_metadata("module-y");

	double default_x;
	double default_y;
	canvas->get_new_module_location(default_x, default_y);
	const double x = (x_str == "") ? default_x : atof(x_str.c_str());
	const double y = (y_str == "") ? default_y : atof(y_str.c_str());

	assert(canvas);
	assert(port_model());

	if (m_module)
		delete m_module;

	m_module = new OmPortModule(canvas, this, x, y);
	
	if (CountedPtr<PatchModel>(port_model()->parent())) {
		if (m_patch_port)
			delete m_patch_port;

		m_patch_port = new OmPatchPort(m_module, port_model());
	}
	
	m_module->resize();

	m_module->move_to(x, y); // FIXME: redundant (?)
}


void
PortController::destroy_module()
{
	delete m_module;
	m_module = NULL;
}


void
PortController::metadata_update(const string& key, const string& value)
{
	//cerr << "Metadata " << path() << ": " << key << " = " << value << endl;

	if (m_module != NULL) {
		if (key == "module-x") {
			float x = atof(value.c_str());
			//if (x > 0 && x < m_canvas->width())
				m_module->move_to(x, m_module->property_y().get_value());
		} else if (key == "module-y") {
			float y = atof(value.c_str());
			//if (y > 0 && y < m_canvas->height())
				m_module->move_to(m_module->property_x().get_value(), y);
		}
	}

	GtkObjectController::metadata_update(key, value);
}

void
PortController::set_path(const Path& new_path)
{
	// Change port name on module, if view exists
	if (m_port != NULL)
		m_port->set_name(new_path.name());

	m_model->set_path(new_path);
}


/** Create the visible port on a canvas module.
 *
 * Does not resize the module, caller's responsibility to do so if necessary.
 */
void
PortController::create_port(OmModule* module)
{
	assert(module != NULL);

	new OmPort(module, port_model());
}


} // namespace Ingenuity

