/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_GUI_PORT_HPP
#define INGEN_GUI_PORT_HPP

#include <cassert>
#include <string>
#include "flowcanvas/Port.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/WeakPtr.hpp"

namespace Raul { class Atom; class URI; }

namespace Ingen {

namespace Client { class PortModel; }
using Ingen::Client::PortModel;

namespace GUI {

class App;

/** A Port on an Module.
 *
 * \ingroup GUI
 */
class Port : public FlowCanvas::Port
{
public:
	static Port* create(
		App&                       app,
		FlowCanvas::Module&        module,
		SharedPtr<const PortModel> pm,
		bool                       human_name,
		bool                       flip = false);

	~Port();

	SharedPtr<const PortModel> model() const { return _port_model.lock(); }

	bool show_menu(GdkEventButton* ev);
	void update_metadata();

	virtual void set_control(float value, bool signal);
	void value_changed(const Raul::Atom& value);
	void activity(const Raul::Atom& value);

	void set_selected(bool b);

	ArtVpathDash* dash();

private:
	Port(App&                       app,
	     FlowCanvas::Module&        module,
	     SharedPtr<const PortModel> pm,
	     const std::string&         name,
	     bool                       flip = false);

	void property_changed(const Raul::URI& key, const Raul::Atom& value);

	bool on_event(GdkEvent* ev);
	void moved();

	static ArtVpathDash* _dash;

	App&                     _app;
	WeakPtr<const PortModel> _port_model;
	bool                     _pressed : 1;
	bool                     _flipped : 1;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PORT_HPP
