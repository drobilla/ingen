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

#include "config.h"
#include <iostream>
#include <string>
#include <signal.h>
#include <dlfcn.h>
#include <boost/optional.hpp>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>
#include <glibmm/thread.h>
#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"
#include "redlandmm/World.hpp"
#include "module/global.hpp"
#include "module/Module.hpp"
#include "module/World.hpp"
#include "engine/tuning.hpp"
#include "engine/Engine.hpp"
#include "engine/QueuedEngineInterface.hpp"
#include "engine/JackAudioDriver.hpp"
#include "serialisation/Parser.hpp"
#include "cmdline.h"

#ifdef HAVE_LIBLO
#include "engine/OSCEngineReceiver.hpp"
#endif
#ifdef HAVE_SOUP
#include "engine/HTTPEngineReceiver.hpp"
#endif

#ifdef WITH_BINDINGS
#include "bindings/ingen_bindings.hpp"
#endif


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
		cmdline_parser_print_help();
		cerr << endl << "*** Ingen requires at least one command line parameter" << endl;
		cerr << "*** Just want a graphical application?  Try 'ingen -eg'" << endl;
		return 1;
	} else if (args.connect_given && args.engine_flag) {
		cerr << "\n*** Nonsense arguments, can't both run a local engine "
				<< "and connect to a remote one." << endl
				<< "*** Run separate instances if that is what you want" << endl;
		return 1;
	}

	SharedPtr<Glib::Module> engine_module;
	SharedPtr<Glib::Module> engine_http_module;
	SharedPtr<Glib::Module> engine_osc_module;
	SharedPtr<Glib::Module> engine_queued_module;
	SharedPtr<Glib::Module> engine_jack_module;
	SharedPtr<Glib::Module> client_module;
	SharedPtr<Glib::Module> gui_module;
	SharedPtr<Glib::Module> bindings_module;

	SharedPtr<Shared::EngineInterface> engine_interface;

	Glib::thread_init();
#if HAVE_SOUP
	g_type_init();
#endif

	Ingen::Shared::World* world = Ingen::Shared::get_world();

	/* Set up RDF world */
	world->rdf_world->add_prefix("xsd", "http://www.w3.org/2001/XMLSchema#");
	world->rdf_world->add_prefix("ingen", "http://drobilla.net/ns/ingen#");
	world->rdf_world->add_prefix("ingenuity", "http://drobilla.net/ns/ingenuity#");
	world->rdf_world->add_prefix("lv2", "http://lv2plug.in/ns/lv2core#");
	world->rdf_world->add_prefix("lv2ev", "http://lv2plug.in/ns/ext/event#");
	world->rdf_world->add_prefix("lv2var", "http://lv2plug.in/ns/ext/instance-var#");
	world->rdf_world->add_prefix("lv2_midi", "http://lv2plug.in/ns/ext/midi");
	world->rdf_world->add_prefix("rdfs", "http://www.w3.org/2000/01/rdf-schema#");
	world->rdf_world->add_prefix("doap", "http://usefulinc.com/ns/doap#");
	world->rdf_world->add_prefix("dc", "http://purl.org/dc/elements/1.1/");

	/* Run engine */
	if (args.engine_flag) {
		engine_module = Ingen::Shared::load_module("ingen_engine");
		engine_http_module = Ingen::Shared::load_module("ingen_engine_http");
		engine_osc_module = Ingen::Shared::load_module("ingen_engine_osc");
		engine_queued_module = Ingen::Shared::load_module("ingen_engine_queued");
		engine_jack_module = Ingen::Shared::load_module("ingen_engine_jack");
		if (engine_module) {
			Engine* (*new_engine)(Ingen::Shared::World* world) = NULL;
			if (engine_module->get_symbol("new_engine", (void*&)new_engine)) {
				engine = SharedPtr<Engine>(new_engine(world));
				world->local_engine = engine;
				/* Load queued (direct in-process) engine interface */
				if (args.gui_given && engine_queued_module) {
					Ingen::QueuedEngineInterface* (*new_interface)(Ingen::Engine& engine);
					if (engine_osc_module->get_symbol("new_queued_interface", (void*&)new_interface)) {
						SharedPtr<QueuedEngineInterface> interface(new_interface(*engine));
						world->local_engine->set_event_source(interface);
						engine_interface = interface;
						world->engine = engine_interface;
					}
				} else {
					#ifdef HAVE_LIBLO
					if (engine_osc_module) {
						Ingen::OSCEngineReceiver* (*new_receiver)(
								Ingen::Engine& engine, size_t queue_size, uint16_t port);
						if (engine_osc_module->get_symbol("new_osc_receiver", (void*&)new_receiver)) {
							SharedPtr<EventSource> source(new_receiver(*engine,
										pre_processor_queue_size, args.engine_port_arg));
							world->local_engine->set_event_source(source);
						}
					}
					#endif
					#ifdef HAVE_SOUP
					if (engine_http_module) {
						// FIXE: leak
						Ingen::HTTPEngineReceiver* (*new_receiver)(Ingen::Engine& engine, uint16_t port);
						if (engine_http_module->get_symbol("new_http_receiver", (void*&)new_receiver)) {
							HTTPEngineReceiver* receiver = new_receiver(
									*world->local_engine, args.engine_port_arg);
							receiver->activate();
						}
					}
					#endif
				}
			} else {
				engine_module.reset();
			}
		} else {
			cerr << "Unable to load engine module." << endl;
		}
	}
	
	/* Load client library */
	if (args.load_given || args.gui_given) {
		client_module = Ingen::Shared::load_module("ingen_client");
		if (!client_module)
			cerr << "Unable to load client module." << endl;
	}
				
	/* If we don't have a local engine interface (for GUI), use network */
	if (client_module && ! engine_interface) {
		SharedPtr<Shared::EngineInterface> (*new_remote_interface)(const std::string&) = NULL;

		if (client_module->get_symbol("new_remote_interface", (void*&)new_remote_interface)) {
			engine_interface = new_remote_interface(args.connect_arg);
		} else {
			cerr << "Unable to find symbol 'new_remote_interface' in "
					"ingen_client module, aborting." << endl;
			return -1;
		}
	}
	
	/* Activate the engine, if we have one */
	if (engine) {
		Ingen::JackAudioDriver* (*new_driver)(
				Ingen::Engine&    engine,
				const std::string server_name,
				const std::string client_name,
				void*             jack_client) = NULL;
		if (engine_jack_module->get_symbol("new_jack_audio_driver", (void*&)new_driver)) {
			engine->set_driver(DataType::AUDIO, SharedPtr<Driver>(new_driver(
					*engine, "default", args.jack_name_arg, NULL)));
		} else {
			cerr << Glib::Module::get_last_error() << endl;
		}

		engine->activate(args.parallelism_arg);
	}
            
	world->engine = engine_interface;

	/* Load a patch */
	if (args.load_given && engine_interface) {
		
		Glib::ustring engine_base = "/";
		if (args.path_given)
			engine_base = string(args.path_arg) + "/";

		bool found = false;
		if (!world->serialisation_module)
			world->serialisation_module = Ingen::Shared::load_module("ingen_serialisation");
			
		Serialisation::Parser* (*new_parser)() = NULL;

		if (world->serialisation_module)
			found = world->serialisation_module->get_symbol("new_parser", (void*&)new_parser);
		
		if (world->serialisation_module && found) {
			SharedPtr<Serialisation::Parser> parser(new_parser());
			
			// Assumption:  Containing ':' means URI, otherwise filename
			string uri = args.load_arg;
			if (uri.find(':') == string::npos) {
				if (Glib::path_is_absolute(args.load_arg))
					uri = Glib::filename_to_uri(args.load_arg);
				else
					uri = Glib::filename_to_uri(Glib::build_filename(
						Glib::get_current_dir(), args.load_arg));
			}


			engine_interface->load_plugins();
			parser->parse_document(world, engine_interface.get(), uri, engine_base, uri);

		} else {
			cerr << "Unable to load serialisation module, aborting." << endl;
			return -1;
		}
	}
	

	/* Run GUI */
	bool ran_gui = false;
	if (args.gui_given) {
		gui_module = Ingen::Shared::load_module("ingen_gui");
		void (*run)(int, char**, Ingen::Shared::World*);
		bool found = gui_module->get_symbol("run", (void*&)run);

		if (found) {
			ran_gui = true;
			run(argc, argv, world);
		} else {
			cerr << "Unable to find GUI module, GUI not loaded." << endl;
		}
	}

    /* Run a script */
    if (args.run_given) {
#ifdef WITH_BINDINGS
        bool (*run_script)(Ingen::Shared::World*, const char*) = NULL;
		SharedPtr<Glib::Module> bindings_module = Ingen::Shared::load_module("ingen_bindings");
        if (!bindings_module)
            cerr << Glib::Module::get_last_error() << endl;
       
        bindings_module->make_resident();

        bool found = bindings_module->get_symbol("run", (void*&)(run_script));
        if (found) {
			setenv("PYTHONPATH", "../../bindings", 1);
			run_script(world, args.run_arg);
        } else {
			cerr << "FAILED: " << Glib::Module::get_last_error() << endl;
        }
#else
		cerr << "This build of ingen does not support scripting." << endl;
#endif
	
	/* Listen to OSC and do our own main thing. */
    } else if (engine && !ran_gui) {
		signal(SIGINT, catch_int);
		signal(SIGTERM, catch_int);
		engine->main();
	}
		
	cout << "Exiting." << endl;

	if (engine) {
		engine->deactivate();
		engine.reset();
	}

	engine_interface.reset();
	client_module.reset();
	world->serialisation_module.reset();
	gui_module.reset();
	engine_module.reset();

	Ingen::Shared::destroy_world();

	return 0;
}

