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

#ifndef PATCHPORTMODULE_H
#define PATCHPORTMODULE_H

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <libgnomecanvasmm.h>
#include "flowcanvas/Module.hpp"
#include "Port.hpp"

namespace Raul { class Atom; }

namespace Ingen { namespace Client {
	class PortModel;
	class NodeModel;
} }
using namespace Ingen::Client;

namespace Ingen {
namespace GUI {
	
class PatchCanvas;
class Port;
class PortMenu;


/** A "module" to represent a patch's port on it's own canvas.
 *
 * Translation: This is the nameless single port pseudo module thingy.
 *
 * \ingroup GUI
 */
class PatchPortModule : public FlowCanvas::Module
{
public:
	static boost::shared_ptr<PatchPortModule> create(boost::shared_ptr<PatchCanvas> canvas,
	                                                 SharedPtr<PortModel>           model,
	                                                 bool                           human);
	
	virtual void store_location();
	void show_human_names(bool b);
	
	void set_name(const std::string& n);

	SharedPtr<PortModel> port() const { return _model; }

protected:
	PatchPortModule(boost::shared_ptr<PatchCanvas> canvas, SharedPtr<PortModel> model);

	void create_menu();
	void set_selected(bool b);

	void set_port(SharedPtr<Port> port) { _port = port; }
	
	void set_variable(const std::string& predicate, const Raul::Atom& value);
	void set_property(const std::string& predicate, const Raul::Atom& value);

	SharedPtr<PortModel> _model;
	SharedPtr<Port>      _port;
	PortMenu*            _menu;
	bool                 _human_name_visible;
};


} // namespace GUI
} // namespace Ingen

#endif // PATCHPORTMODULE_H
