/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <signal.h>
#include <stdlib.h>

#include <iostream>
#include <string>

#include <glibmm/thread.h>
#include <glibmm/timer.h>

#include "raul/Path.hpp"

#include "ingen_config.h"

#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Log.hpp"
#include "ingen/Parser.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ThreadedSigClientInterface.hpp"
#include "ingen/runtime_paths.hpp"
#include "ingen/types.hpp"
#ifdef HAVE_SOCKET
#include "ingen/client/SocketClient.hpp"
#endif

using namespace std;
using namespace Ingen;

Ingen::World* world = NULL;

static void
ingen_interrupt(int signal)
{
	if (signal == SIGTERM) {
		cerr << "ingen: Terminated" << endl;
		delete world;
		exit(EXIT_FAILURE);
	} else {
		cout << "ingen: Interrupted" << endl;
		if (world && world->engine()) {
			world->engine()->quit();
		}
	}
}

static void
ingen_try(bool cond, const char* msg)
{
	if (!cond) {
		cerr << "ingen: error: " << msg << endl;
		delete world;
		exit(EXIT_FAILURE);
	}
}

static int
print_version()
{
	cout << "ingen " << INGEN_VERSION
	     << " <http://drobilla.net/software/ingen>\n"
	     << "Copyright 2007-2017 David Robillard <http://drobilla.net>.\n"
	     << "License: <https://www.gnu.org/licenses/agpl-3.0>\n"
	     << "This is free software; you are free to change and redistribute it.\n"
	     << "There is NO WARRANTY, to the extent permitted by law." << endl;
	return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
	Glib::thread_init();
	Ingen::set_bundle_path_from_code((void*)&print_version);

	// Create world
	try {
		world = new Ingen::World(NULL, NULL, NULL);
		world->load_configuration(argc, argv);
		if (argc <= 1) {
			world->conf().print_usage("ingen", cout);
			return EXIT_FAILURE;
		} else if (world->conf().option("help").get<int32_t>()) {
			world->conf().print_usage("ingen", cout);
			return EXIT_SUCCESS;
		} else if (world->conf().option("version").get<int32_t>()) {
			return print_version();
		}
	} catch (std::exception& e) {
		cout << "ingen: error: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	Configuration& conf = world->conf();
	if (conf.option("uuid").is_valid()) {
		world->set_jack_uuid(conf.option("uuid").ptr<char>());
	}

	// Run engine
	SPtr<Interface> engine_interface;
	if (conf.option("engine").get<int32_t>()) {
		ingen_try(world->load_module("server"), "Failed to load server module");

		ingen_try(bool(world->engine()), "Unable to create engine");
		world->engine()->listen();

		engine_interface = world->interface();
	}

	// If we don't have a local engine interface (for GUI), use network
	if (!engine_interface) {
		ingen_try(world->load_module("client"), "Failed to load client module");
#ifdef HAVE_SOCKET
		Client::SocketClient::register_factories(world);
#endif
		const char* const uri = conf.option("connect").ptr<char>();
		ingen_try(Raul::URI::is_valid(uri),
		          (fmt("Invalid URI <%1%>") % uri).str().c_str());
		SPtr<Interface> client(new Client::ThreadedSigClientInterface(1024));
		engine_interface = world->new_interface(Raul::URI(uri), client);

		if (!engine_interface && !conf.option("gui").get<int32_t>()) {
			cerr << (fmt("ingen: error: Failed to connect to `%1%'\n") % uri);
			delete world;
			return EXIT_FAILURE;
		}
	}

	world->set_interface(engine_interface);

	// Load GUI if requested
	if (conf.option("gui").get<int32_t>()) {
		ingen_try(world->load_module("gui"), "Failed to load GUI module");
	}

	// Activate the engine, if we have one
	if (world->engine()) {
		if (!world->load_module("jack") && !world->load_module("portaudio")) {
			cerr << "ingen: error: Failed to load driver module" << endl;
			delete world;
			exit(EXIT_FAILURE);
		}
	}

	// Load a graph
	if (conf.option("load").is_valid()) {
		boost::optional<Raul::Path>   parent;
		boost::optional<Raul::Symbol> symbol;

		const Atom& path_option = conf.option("path");
		if (path_option.is_valid()) {
			if (Raul::Path::is_valid(path_option.ptr<char>())) {
				const Raul::Path p(path_option.ptr<char>());
				if (!p.is_root()) {
					parent = p.parent();
					symbol = Raul::Symbol(p.symbol());
				}
			} else {
				cerr << "Invalid path given: '" << path_option.ptr<char>() << endl;
			}
		}

		ingen_try(bool(world->parser()), "Failed to create parser");

		const string graph = conf.option("load").ptr<char>();

		engine_interface->get(Raul::URI("ingen:/plugins"));
		engine_interface->get(main_uri());

		std::lock_guard<std::mutex> lock(world->rdf_mutex());
		world->parser()->parse_file(
			world, engine_interface.get(), graph, parent, symbol);
	} else if (conf.option("server-load").is_valid()) {
		const char* path = conf.option("server-load").ptr<char>();
		if (serd_uri_string_has_scheme((const uint8_t*)path)) {
			std::cout << "Loading " << path << " (server side)" << std::endl;
			engine_interface->copy(Raul::URI(path), main_uri());
		} else {
			SerdNode uri = serd_node_new_file_uri(
				(const uint8_t*)path, NULL, NULL, true);
			std::cout << "Loading " << (const char*)uri.buf
			          << " (server side)" << std::endl;
			engine_interface->copy(Raul::URI((const char*)uri.buf),
			                       main_uri());
			serd_node_free(&uri);
		}
	}

	// Save the currently loaded graph
	if (conf.option("save").is_valid()) {
		const char* path = conf.option("save").ptr<char>();
		if (serd_uri_string_has_scheme((const uint8_t*)path)) {
			std::cout << "Saving to " << path << std::endl;
			engine_interface->copy(main_uri(), Raul::URI(path));
		} else {
			SerdNode uri = serd_node_new_file_uri(
				(const uint8_t*)path, NULL, NULL, true);
			std::cout << "Saving to " << (const char*)uri.buf << std::endl;
			engine_interface->copy(main_uri(),
			                       Raul::URI((const char*)uri.buf));
			serd_node_free(&uri);
		}
	}

	// Activate the engine now that the graph is loaded
	if (world->engine()) {
		world->engine()->flush_events(std::chrono::milliseconds(10));
		world->engine()->activate();
	}

	// Set up signal handlers that will set quit_flag on interrupt
	signal(SIGINT, ingen_interrupt);
	signal(SIGTERM, ingen_interrupt);

	if (conf.option("gui").get<int32_t>()) {
		world->run_module("gui");
	} else if (world->engine()) {
		// Run engine main loop until interrupt
		while (world->engine()->main_iteration()) {
			Glib::usleep(125000);  // 1/8 second
		}
	}

	// Sleep for a half second to allow event queues to drain
	Glib::usleep(500000);

	// Shut down
	if (world->engine())
		world->engine()->deactivate();

	// Save configuration to restore preferences on next run
	const std::string path = conf.save(
		world->uri_map(), "ingen", "options.ttl", Configuration::GLOBAL);
	std::cout << (fmt("Saved configuration to %1%") % path) << std::endl;

	engine_interface.reset();
	delete world;

	return 0;
}
