/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "cmdline.h"
#include "ConnectWindow.h"
#include "App.h"
#include "Store.h"
#include "Controller.h"
#include "Configuration.h"
#ifdef HAVE_LASH
	#include "LashController.h"
#endif
#include "ThreadedSigClientInterface.h"
#include "OSCListener.h"
using Om::Shared::ClientInterface;

using namespace OmGtk;


class OSCSigEmitter : public OSCListener, public ThreadedSigClientInterface {
public:
	OSCSigEmitter(size_t queue_size, int listen_port)
	: Om::Shared::ClientInterface()
	, OSCListener(listen_port)
	, ThreadedSigClientInterface(queue_size)
	{
		Glib::signal_timeout().connect(
			sigc::mem_fun((ThreadedSigClientInterface*)this,
				&ThreadedSigClientInterface::emit_signals),
			5, G_PRIORITY_DEFAULT_IDLE);
	}
};


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
	
	OSCSigEmitter* emitter = new OSCSigEmitter(1024, 16181);

	/* Instantiate all singletons */
	App::instantiate();
	Store::instantiate(*(SigClientInterface*)emitter);

	Controller::instantiate(engine_url);

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
	
	App::instance().connect_window()->start(CountedPtr<ClientInterface>(emitter));
	gtk_main.run();

	return 0;
}

