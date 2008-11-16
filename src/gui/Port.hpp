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

#ifndef GUI_PORT_H
#define GUI_PORT_H

#include <cassert>
#include <string>
#include "flowcanvas/Port.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/WeakPtr.hpp"
#include "raul/Atom.hpp"

namespace Ingen { namespace Client { class PortModel; } }
using Ingen::Client::PortModel;

namespace Ingen {
namespace GUI {


/** A Port on an Module.
 * 
 * \ingroup GUI
 */
class Port : public FlowCanvas::Port
{
public:
	Port(boost::shared_ptr<FlowCanvas::Module> module,
	     SharedPtr<PortModel>                  pm,
	     const std::string&                    name,
	     bool                                  flip=false);

	~Port();

	SharedPtr<PortModel> model() const { return _port_model.lock(); }

	void create_menu();
	
	virtual void set_control(float value, bool signal);
	void value_changed(const Raul::Atom& value);
	void activity();
	
private:
	
	void variable_changed(const std::string& key, const Raul::Atom& value);

	void renamed();

	WeakPtr<PortModel> _port_model;
	bool               _flipped;
};


} // namespace GUI
} // namespace Ingen

#endif // GUI_PORT_H
