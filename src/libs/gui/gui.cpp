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

#include "gui.hpp"
#include "ConnectWindow.hpp"
#include "App.hpp"
#include "Configuration.hpp"

namespace Ingen {
namespace GUI {


void run(int argc, char** argv,
		SharedPtr<Ingen::Engine> engine,
		SharedPtr<Shared::EngineInterface> interface)
{
	App::run(argc, argv, engine, interface);
#if 0
	Gnome::Canvas::init();
	Gtk::Main gtk_main(argc, argv);
	Gtk::Window::set_default_icon_from_file(PKGDATADIR "/ingen.svg");
	
	/* Instantiate singleton (bad, FIXME) */
	App::instantiate(argc, argv);

	/* Load settings */
	App::instance().configuration()->load_settings();
	App::instance().configuration()->apply_settings();

	App::instance().connect_window()->start();
	gtk_main.run();

	return 0;
#endif
}


} // namespace GUI
} // namespace Ingen

