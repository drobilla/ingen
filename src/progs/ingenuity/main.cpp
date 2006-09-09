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

#include "config.h"
#include "cmdline.h"
#include "ConnectWindow.h"
#include "App.h"
#include "Configuration.h"
#ifdef HAVE_LASH
	#include "LashController.h"
#endif

using namespace Ingenuity;


int
main(int argc, char** argv)
{
	string engine_url  = "osc.udp://localhost:16180";
	int    client_port = 0;

	/* **** Parse command line options **** */
	gengetopt_args_info args_info;
	if (cmdline_parser (argc, argv, &args_info) != 0)
		return 1;

	if (args_info.engine_url_given)
		engine_url = args_info.engine_url_arg;
	if (args_info.client_port_given)
		client_port = args_info.client_port_arg;
	
	// FIXME:
	client_port = 16181;

	Gnome::Canvas::init();
	Gtk::Main gtk_main(argc, argv);
	
	/* Instantiate all singletons */
	App::instantiate();

	/* Load settings */
	App::instance().configuration()->load_settings();
	App::instance().configuration()->apply_settings();

#ifdef HAVE_LASH
	lash_args_t* lash_args = lash_extract_args(&argc, &argv);
#endif
	
	//gtk_main.signal_quit().connect(sigc::ptr_fun(cleanup));		

#ifdef HAVE_LASH
	LashController* lash_controller = new LashController(lash_args);
#endif
	
	App::instance().connect_window()->start();
	gtk_main.run();

	return 0;
}

