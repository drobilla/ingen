/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_GUI_NODECONTROLWINDOW_HPP
#define INGEN_GUI_NODECONTROLWINDOW_HPP

#include <gtkmm.h>
#include <sigc++/sigc++.h>

#include "raul/SharedPtr.hpp"

#include "Window.hpp"

namespace Ingen { namespace Client {
	class NodeModel;
} }

namespace Ingen {
namespace GUI {

class ControlGroup;
class ControlPanel;

/** Window with controls (sliders) for all control-rate ports on a Node.
 *
 * \ingroup GUI
 */
class NodeControlWindow : public Window
{
public:
	NodeControlWindow(App& app, SharedPtr<const Client::NodeModel> node, uint32_t poly);
	NodeControlWindow(App& app, SharedPtr<const Client::NodeModel> node, ControlPanel* panel);

	virtual ~NodeControlWindow();

	SharedPtr<const Client::NodeModel> node() const { return _node; }

	ControlPanel* control_panel() const { return _control_panel; }

	void resize();

protected:
	void on_show();
	void on_hide();

private:
	SharedPtr<const Client::NodeModel> _node;
	ControlPanel*                      _control_panel;
	bool                               _callback_enabled;

	bool _position_stored;
	int  _x;
	int  _y;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_NODECONTROLWINDOW_HPP
