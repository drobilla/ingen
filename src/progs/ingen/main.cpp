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

#include CONFIG_H_PATH
#include <iostream>
#include <string>
#include <signal.h>
#include <dlfcn.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>
#include <boost/optional.hpp>
#include <glibmm/thread.h>
#include <raul/Path.hpp>
#include <raul/RDFWorld.hpp>
#include <raul/SharedPtr.hpp>
#include "module/module.h"
#include "engine/Engine.hpp"
#include "engine/QueuedEngineInterface.hpp"
#include "serialisation/Loader.hpp"
#include "bindings/ingen_bindings.hpp"
#include "cmdline.h"

using namespace std;
using namespace Ingen;


SharedPtr<Engine> engine;

void
catch_int(int)
{
	signal(SIGINT, catch_int);
	signal(SIGTERM, catch_int);

	cout << "[Main] Ingen interrupted." << endl;
	engine->quit();
}

int
main(int argc, char** argv)
{
	/* Parse command line options */
	gengetopt_args_info args;
	if (cmdline_parser (argc, argv, &args) != 0)
		return 1;

	if (argc <= 1) {
		cout << "No arguments provided.  Try something like:" << endl << endl;
		cout << "Run an engine: ingen -e" << endl;
		cout << "Run the GUI:   ingen -g" << endl;
		cout << "Print full help: ingen -h" << endl << endl;
		cmdline_parser_print_help();
		return 0;
	}

	SharedPtr<Glib::Module> engine_module;
	SharedPtr<Glib::Module> client_module;
	SharedPtr<Glib::Module> gui_module;
	SharedPtr<Glib::Module> bindings_module;

	SharedPtr<Shared::EngineInterface> engine_interface;

	Glib::thread_init();

	Ingen::Shared::World* world = Ingen::Shared::get_world();

	/* Run engine */
	if (args.engine_flag) {

		engine_module = Ingen::Shared::load_module("ingen_engine");

		if (engine_module) {
			Engine* (*new_engine)(Ingen::Shared::World* world) = NULL;
			if (engine_module->get_symbol("new_engine", (void*&)new_engine)) {
				engine = SharedPtr<Engine>(new_engine(world));
			} else {
				engine_module.reset();
			}
		} else {
			cerr << "Unable to load engine module, engine not loaded." << endl;
			cerr << "Try using src/set_dev_environment.sh, or setting INGEN_MODULE_PATH." << endl;
		}

	}
	
	bool use_osc = false;

	/* Connect to remote engine */
	if (args.connect_given || (args.load_given && !engine_interface)) {
		bool found = false;
		client_module = Ingen::Shared::load_module("ingen_client");

		SharedPtr<Shared::EngineInterface> (*new_osc_interface)(const std::string&) = NULL;

		if (client_module)
			found = client_module->get_symbol("new_osc_interface", (void*&)new_osc_interface);

		if (client_module && found) {
			engine_interface = new_osc_interface(args.connect_arg);
			use_osc = true;
		} else {
			cerr << "Unable to load ingen_client module, aborting." << endl;
			cerr << "Try using src/set_dev_environment.sh, or setting INGEN_MODULE_PATH." << endl;
			return -1;
		}
	}
	
	/* Load queued (direct in-process) engine interface */
	if (engine && !engine_interface && (args.load_given || args.gui_given || args.run_given)) {
		engine_interface = engine->new_queued_interface();
    }

	if (engine && engine_interface) {

		if (use_osc)
			engine->start_osc_driver(args.engine_port_arg);

		engine->start_jack_driver();

		engine->activate();
	}
            
	world->engine = engine_interface.get();

	/* Load a patch */
	if (args.load_given && engine_interface) {
		
		Raul::RDF::World rdf_world;
		rdf_world.add_prefix("xsd", "http://www.w3.org/2001/XMLSchema#");
		rdf_world.add_prefix("ingen", "http://drobilla.net/ns/ingen#");
		rdf_world.add_prefix("ingenuity", "http://drobilla.net/ns/ingenuity#");
		rdf_world.add_prefix("lv2", "http://lv2plug.in/ns/lv2core#");
		rdf_world.add_prefix("rdfs", "http://www.w3.org/2000/01/rdf-schema#");
		rdf_world.add_prefix("doap", "http://usefulinc.com/ns/doap#");

		boost::optional<Raul::Path> parent_path;
		if (args.path_given)
			parent_path = args.path_arg;

		bool found = false;
		SharedPtr<Glib::Module> serialisation_module
			= Ingen::Shared::load_module("ingen_serialisation");
			
		Serialisation::Loader* (*new_loader)() = NULL;

		if (serialisation_module)
			found = serialisation_module->get_symbol("new_loader", (void*&)new_loader);
		
		if (serialisation_module && found) {
			SharedPtr<Serialisation::Loader> loader(new_loader());
			
			// Assumption:  Containing ':' means URI, otherwise filename
			string uri = args.load_arg;
			if (uri.find(':') == string::npos) 
				if (Glib::path_is_absolute(args.load_arg))
					uri = Glib::filename_to_uri(args.load_arg);
				else
					uri = Glib::filename_to_uri(Glib::build_filename(
						Glib::get_current_dir(), args.load_arg));

			loader->load(engine_interface, &rdf_world, uri, parent_path, "");

		} else {
			cerr << "Unable to load serialisation module, aborting." << endl;
			cerr << "Try using src/set_dev_environment.sh, or setting INGEN_MODULE_PATH." << endl;
			return -1;
		}
	}
	

	/* Run GUI */
	bool ran_gui = false;
	if (args.gui_given) {
		gui_module = Ingen::Shared::load_module("ingen_gui");
		void (*run)(int, char**, Ingen::Shared::World*, SharedPtr<Ingen::Engine>, SharedPtr<Shared::EngineInterface>) = NULL;
		bool found = gui_module->get_symbol("run", (void*&)run);

		if (found) {
			ran_gui = true;
			run(argc, argv, world, engine, engine_interface);
		} else {
			cerr << "Unable to find GUI module, GUI not loaded." << endl;
			cerr << "Try using src/set_dev_environment.sh, or setting INGEN_MODULE_PATH." << endl;
		}
	}

    /* Run a script */
    if (args.run_given) {
        bool (*run_script)(Ingen::Shared::World*, const char*) = NULL;
		SharedPtr<Glib::Module> bindings_module = Ingen::Shared::load_module("ingen_bindings");
        if (!bindings_module)
            cerr << Glib::Module::get_last_error() << endl;
       
        bindings_module->make_resident();

        bool found = bindings_module->get_symbol("run", (void*&)(run_script));
        if (found) {
			cerr << "WORLD: " << world << endl;
			cerr << "ENGINE: " << world->engine << endl;
			setenv("PYTHONPATH", "../../bindings", 1);
			run_script(world, args.run_arg);
        } else {
			cerr << "FAILED: " << Glib::Module::get_last_error() << endl;
        }
	
	/* Listen to OSC and do our own main thing. */
    } else if (engine && !ran_gui) {

		signal(SIGINT, catch_int);
		signal(SIGTERM, catch_int);
		
		engine->start_osc_driver(args.engine_port_arg);
		engine->start_jack_driver();
		engine->activate();

		engine->main();
	}
		
	cout << "Exiting." << endl;

	if (engine) {
		engine->deactivate();
		engine.reset();
	}

	engine_module.reset();
	client_module.reset();
	gui_module.reset();

	Ingen::Shared::destroy_world();

	return 0;
}

