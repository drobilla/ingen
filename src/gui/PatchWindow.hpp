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

#ifndef INGEN_GUI_PATCH_WINDOW_HPP
#define INGEN_GUI_PATCH_WINDOW_HPP

#include <gtkmm.h>

#include "raul/SharedPtr.hpp"

#include "PatchBox.hpp"
#include "Window.hpp"

namespace Ingen {

namespace Client {
	class PatchModel;
}
using namespace Ingen::Client;

namespace GUI {

/** A window for a patch.
 *
 * \ingroup GUI
 */
class PatchWindow : public Window
{
public:
	PatchWindow(BaseObjectType*                   cobject,
	            const Glib::RefPtr<Gtk::Builder>& xml);

	~PatchWindow();

	void init_window(App& app);

	SharedPtr<const PatchModel> patch() const { return _box->patch(); }
	PatchBox*                   box()   const { return _box; }

	void show_documentation(const std::string& doc, bool html) {
		_box->show_documentation(doc, html);
	}

	void hide_documentation() {
		_box->hide_documentation();
	}

	void show_port_status(const PortModel* model, const Raul::Atom& value) {
		_box->show_port_status(model, value);
	}

protected:
	void on_hide();
	void on_show();

private:
	PatchBox* _box;
	bool      _position_stored;
	int       _x;
	int       _y;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_PATCH_WINDOW_HPP
