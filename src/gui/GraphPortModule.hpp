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

#include "Port.hpp"

#include "ganv/Module.hpp"

#include <memory>
#include <string>

namespace ingen {
namespace client {
class PortModel;
} // namespace client
} // namespace ingen

namespace ingen {
namespace gui {

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
	static GraphPortModule*
	create(GraphCanvas&                                    canvas,
	       const std::shared_ptr<const client::PortModel>& model);

	App& app() const;

	virtual void store_location(double ax, double ay);
	void show_human_names(bool b);

	void set_name(const std::string& n);

	std::shared_ptr<const client::PortModel> port() const { return _model; }

protected:
	GraphPortModule(GraphCanvas&                                    canvas,
	                const std::shared_ptr<const client::PortModel>& model);

	bool show_menu(GdkEventButton* ev);
	void set_selected(gboolean b) override;

	void set_port(Port* port) { _port = port; }

	void property_changed(const URI& key, const Atom& value);

	std::shared_ptr<const client::PortModel> _model;
	Port*                                    _port;
};

} // namespace gui
} // namespace ingen

#endif // INGEN_GUI_GRAPHPORTMODULE_HPP
