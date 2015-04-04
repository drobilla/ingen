/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_GRAPHPORTMODULE_HPP
#define INGEN_GUI_GRAPHPORTMODULE_HPP

#include <string>

#include "ganv/Module.hpp"
#include "raul/URI.hpp"

#include "Port.hpp"

namespace Raul { class Atom; }

namespace Ingen { namespace Client {
class PortModel;
} }

namespace Ingen {
namespace GUI {

class GraphCanvas;
class Port;
class PortMenu;

/** A "module" to represent a graph's port on its own canvas.
 *
 * Translation: This is the nameless single port pseudo module thingy.
 *
 * \ingroup GUI
 */
class GraphPortModule : public Ganv::Module
{
public:
	static GraphPortModule* create(
		GraphCanvas&                  canvas,
		SPtr<const Client::PortModel> model);

	App& app() const;

	virtual void store_location(double x, double y);
	void show_human_names(bool b);

	void set_name(const std::string& n);

	SPtr<const Client::PortModel> port() const { return _model; }

protected:
	GraphPortModule(GraphCanvas&                  canvas,
	                SPtr<const Client::PortModel> model);

	bool show_menu(GdkEventButton* ev);
	void set_selected(gboolean b);

	void set_port(Port* port) { _port = port; }

	void property_changed(const Raul::URI& predicate, const Atom& value);

	SPtr<const Client::PortModel> _model;
	Port*                         _port;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_GRAPHPORTMODULE_HPP
