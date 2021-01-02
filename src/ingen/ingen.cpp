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

#include "ingen/Atom.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/FilePath.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"
#include "ingen/Parser.hpp"
#include "ingen/URI.hpp"
#include "ingen/World.hpp"
#include "ingen/fmt.hpp"
#include "ingen/paths.hpp"
#include "ingen/runtime_paths.hpp"
#include "ingen_config.h"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"
#include "serd/serd.h"

#ifdef HAVE_SOCKET
#include "ingen/client/SocketClient.hpp"
#endif

#include <boost/optional/optional.hpp>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace ingen {
namespace {

class DummyInterface : public Interface
{
	URI  uri() const override { return URI("ingen:dummy"); }
	void message(const Message& msg) override {}
};

std::unique_ptr<World> world;

void
ingen_interrupt(int signal)
{
	if (signal == SIGTERM) {
		std::cerr << "ingen: Terminated\n";
		exit(EXIT_FAILURE);
	} else {
		std::cout << "ingen: Interrupted\n";
		if (world && world->engine()) {
			world->engine()->quit();
		}
	}
}

void
ingen_try(bool cond, const char* msg)
{
	if (!cond) {
		std::cerr << "ingen: error: " << msg << "\n";
		exit(EXIT_FAILURE);
	}
}

int
print_version()
{
	std::cout << "ingen " << INGEN_VERSION
	          << " <http://drobilla.net/software/ingen>\n"
	          << "Copyright 2007-2020 David Robillard <http://drobilla.net>.\n"
	          << "License: <https://www.gnu.org/licenses/agpl-3.0>\n"
	          << "This is free software; you are free to change and redistribute it.\n"
	          << "There is NO WARRANTY, to the extent permitted by law.\n";
	return EXIT_SUCCESS;
}

int
run(int argc, char** argv)
{
	// Create world
	try {
		world = std::unique_ptr<ingen::World>(
			new ingen::World(nullptr, nullptr, nullptr));
		world->load_configuration(argc, argv);
		if (argc <= 1) {
			world->conf().print_usage("ingen", std::cout);
			return EXIT_FAILURE;
		} else if (world->conf().option("help").get<int32_t>()) {
			world->conf().print_usage("ingen", std::cout);
			return EXIT_SUCCESS;
		} else if (world->conf().option("version").get<int32_t>()) {
			return print_version();
		}
	} catch (std::exception& e) {
		std::cout << "ingen: error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	Configuration& conf = world->conf();
	if (conf.option("uuid").is_valid()) {
		world->set_jack_uuid(conf.option("uuid").ptr<char>());
	}

	// Run engine
	if (conf.option("engine").get<int32_t>()) {
		if (world->conf().option("threads").get<int32_t>() < 1) {
			std::cerr << "ingen: error: threads must be > 0\n";
			return EXIT_FAILURE;
		}

		ingen_try(world->load_module("server"), "Failed to load server module");

		ingen_try(bool(world->engine()), "Unable to create engine");
		world->engine()->listen();
	}

#ifdef HAVE_SOCKET
	client::SocketClient::register_factories(*world);
#endif

	// Load GUI if requested
	if (conf.option("gui").get<int32_t>()) {
		ingen_try(world->load_module("client"), "Failed to load client module");
		ingen_try(world->load_module("gui"), "Failed to load GUI module");
	}

	// If we don't have a local engine interface (from the GUI), use network
	auto engine_interface = world->interface();
	auto dummy_client     = std::make_shared<DummyInterface>();
	if (!engine_interface) {
		const char* const uri = conf.option("connect").ptr<char>();
		ingen_try(URI::is_valid(uri),
		          fmt("Invalid URI <%1%>", uri).c_str());
		engine_interface = world->new_interface(URI(uri), dummy_client);

		if (!engine_interface && !conf.option("gui").get<int32_t>()) {
			std::cerr << fmt("ingen: error: Failed to connect to `%1%'\n", uri);
			return EXIT_FAILURE;
		}

		world->set_interface(engine_interface);
	}

	// Activate the engine, if we have one
	if (world->engine()) {
		if (!world->load_module("jack") && !world->load_module("portaudio")) {
			std::cerr << "ingen: error: Failed to load driver module\n";
			return EXIT_FAILURE;
		}

		if (!world->engine()->supports_dynamic_ports() &&
		    !conf.option("load").is_valid()) {
			std::cerr << "ingen: error: Initial graph required for driver\n";
			return EXIT_FAILURE;
		}
	}

	// Load a graph
	if (conf.option("load").is_valid()) {
		boost::optional<raul::Path>   parent;
		boost::optional<raul::Symbol> symbol;

		const Atom& path_option = conf.option("path");
		if (path_option.is_valid()) {
			if (raul::Path::is_valid(path_option.ptr<char>())) {
				const raul::Path p(path_option.ptr<char>());
				if (!p.is_root()) {
					parent = p.parent();
					symbol = raul::Symbol(p.symbol());
				}
			} else {
				std::cerr << "Invalid path given: '" << path_option.ptr<char>()
				          << "\n";
			}
		}

		ingen_try(bool(world->parser()), "Failed to create parser");

		const std::string graph = conf.option("load").ptr<char>();

		engine_interface->get(URI("ingen:/plugins"));
		engine_interface->get(main_uri());

		std::lock_guard<std::mutex> lock(world->rdf_mutex());
		world->parser()->parse_file(
			*world, *engine_interface, graph, parent, symbol);
	} else if (conf.option("server-load").is_valid()) {
		const char* path = conf.option("server-load").ptr<char>();
		if (serd_uri_string_has_scheme(reinterpret_cast<const uint8_t*>(path))) {
			std::cout << "Loading " << path << " (server side)" << "\n";
			engine_interface->copy(URI(path), main_uri());
		} else {
			SerdNode uri = serd_node_new_file_uri(
				reinterpret_cast<const uint8_t*>(path), nullptr, nullptr, true);
			std::cout << "Loading " << reinterpret_cast<const char*>(uri.buf)
			          << " (server side)\n";
			engine_interface->copy(URI(reinterpret_cast<const char*>(uri.buf)),
			                       main_uri());
			serd_node_free(&uri);
		}
	}

	// Save the currently loaded graph
	if (conf.option("save").is_valid()) {
		const char* path = conf.option("save").ptr<char>();
		if (serd_uri_string_has_scheme(reinterpret_cast<const uint8_t*>(path))) {
			std::cout << "Saving to " << path << "\n";
			engine_interface->copy(main_uri(), URI(path));
		} else {
			SerdNode uri = serd_node_new_file_uri(
				reinterpret_cast<const uint8_t*>(path), nullptr, nullptr, true);
			std::cout << "Saving to " << reinterpret_cast<const char*>(uri.buf)
			          << "\n";
			engine_interface->copy(main_uri(), URI(reinterpret_cast<const char*>(uri.buf)));
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
			std::this_thread::sleep_for(std::chrono::milliseconds(125));
		}
	}

	// Sleep for a half second to allow event queues to drain
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Shut down
	if (world->engine()) {
		world->engine()->deactivate();
	}

	// Save configuration to restore preferences on next run
	const std::string path = conf.save(
		world->uri_map(), "ingen", "options.ttl", Configuration::GLOBAL);
	std::cout << fmt("Saved configuration to %1%\n", path);

	engine_interface.reset();

	return 0;
}

} // namespace
} // namespace ingen

int
main(int argc, char** argv)
{
	ingen::set_bundle_path_from_code(
		reinterpret_cast<void (*)()>(&ingen::run));

	return ingen::run(argc, argv);
}
