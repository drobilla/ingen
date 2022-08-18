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

#include "TestClient.hpp"

#include "ingen/Atom.hpp"
#include "ingen/AtomForge.hpp"
#include "ingen/AtomReader.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/FilePath.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/Store.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "ingen/filesystem.hpp"
#include "ingen/fmt.hpp"
#include "ingen/memory.hpp"
#include "ingen/runtime_paths.hpp"
#include "raul/Path.hpp"
#include "serd/serd.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace ingen {
namespace test {
namespace {

std::unique_ptr<World> world;

void
ingen_try(bool cond, const char* msg)
{
	if (!cond) {
		std::cerr << "ingen: Error: " << msg << std::endl;
		world.reset();
		exit(EXIT_FAILURE);
	}
}

FilePath
real_file_path(const char* path)
{
	std::unique_ptr<char, FreeDeleter<char>> real_path{realpath(path, nullptr),
	                                                   FreeDeleter<char>{}};

	return FilePath{real_path.get()};
}

int
run(int argc, char** argv)
{
	// Create world
	try {
		world = std::unique_ptr<World>{new World(nullptr, nullptr, nullptr)};
		world->load_configuration(argc, argv);
	} catch (std::exception& e) {
		std::cout << "ingen: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// Get mandatory command line arguments
	const Atom& load    = world->conf().option("load");
	const Atom& execute = world->conf().option("execute");
	if (!load.is_valid() || !execute.is_valid()) {
		std::cerr
		    << "Usage: ingen_test --load START_GRAPH --execute COMMANDS_FILE"
		    << std::endl;

		return EXIT_FAILURE;
	}

	// Get start graph and commands file options
	const FilePath load_path = real_file_path(static_cast<const char*>(load.get_body()));
	const FilePath run_path  = real_file_path(static_cast<const char*>(execute.get_body()));

	if (load_path.empty()) {
		std::cerr << "error: initial graph '" << load_path << "' does not exist"
		          << std::endl;

		return EXIT_FAILURE;
	}

	if (run_path.empty()) {
		std::cerr << "error: command file '" << run_path << "' does not exist"
		          << std::endl;

		return EXIT_FAILURE;
	}

	// Load modules
	ingen_try(world->load_module("server"),
	          "Unable to load server module");

	// Initialise engine
	ingen_try(bool(world->engine()),
	          "Unable to create engine");
	world->engine()->init(48000.0, 4096, 4096);
	world->engine()->activate();

	// Load graph
	if (!world->parser()->parse_file(*world, *world->interface(), load_path)) {
		std::cerr << "error: failed to load initial graph " << load_path
		          << std::endl;

		return EXIT_FAILURE;
	}
	world->engine()->flush_events(std::chrono::milliseconds(20));

	// Read commands

	AtomForge forge(world->uri_map().urid_map());

	sratom_set_object_mode(&forge.sratom(), SRATOM_OBJECT_MODE_BLANK_SUBJECT);

	// AtomReader to read commands from a file and send them to engine
	AtomReader atom_reader(world->uri_map(),
	                       world->uris(),
	                       world->log(),
	                       *world->interface());

	// AtomWriter to serialise responses from the engine
	std::shared_ptr<Interface> client(new TestClient(world->log()));

	world->interface()->set_respondee(client);
	world->engine()->register_client(client);

	SerdURI cmds_base;
	SerdNode cmds_file_uri = serd_node_new_file_uri(
		reinterpret_cast<const uint8_t*>(run_path.c_str()),
		nullptr, &cmds_base, true);

	auto* cmds =
	    new Sord::Model(*world->rdf_world(),
	                    reinterpret_cast<const char*>(cmds_file_uri.buf));

	SerdEnv* env = serd_env_new(&cmds_file_uri);
	cmds->load_file(env, SERD_TURTLE, run_path);
	Sord::Node nil;
	int n_events = 0;
	for (;; ++n_events) {
		std::string subject_str = fmt("msg%1%", n_events);
		Sord::URI subject(*world->rdf_world(), subject_str,
		                  reinterpret_cast<const char*>(cmds_file_uri.buf));

		auto iter = cmds->find(subject, nil, nil);
		if (iter.end()) {
			break;
		}

		forge.clear();
		forge.read(*world->rdf_world(), cmds->c_obj(), subject.c_obj());

#if 0
		const LV2_Atom* atom = forge.atom();
		cerr << "READ " << atom->size << " BYTES" << endl;
		cerr << sratom_to_turtle(
			sratom,
			&world->uri_map().urid_unmap_feature()->urid_unmap,
			(const char*)cmds_file_uri.buf,
			nullptr, nullptr, atom->type, atom->size, LV2_ATOM_BODY(atom)) << endl;
#endif

		if (!atom_reader.write(forge.atom(), n_events + 1)) {
			return EXIT_FAILURE;
		}

		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	delete cmds;

	// Save resulting graph
	auto              r        = world->store()->find(raul::Path("/"));
	const std::string base     = run_path.stem();
	const std::string out_name = base.substr(0, base.find('.')) + ".out.ingen";
	const FilePath    out_path = filesystem::current_path() / out_name;
	world->serialiser()->write_bundle(r->second, URI(out_path));

	// Undo every event (should result in a graph identical to the original)
	for (int i = 0; i < n_events; ++i) {
		world->interface()->undo();
		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	// Save completely undone graph
	r = world->store()->find(raul::Path("/"));
	const std::string undo_name = base.substr(0, base.find('.')) + ".undo.ingen";
	const FilePath    undo_path = filesystem::current_path() / undo_name;
	world->serialiser()->write_bundle(r->second, URI(undo_path));

	// Redo every event (should result in a graph identical to the pre-undo output)
	for (int i = 0; i < n_events; ++i) {
		world->interface()->redo();
		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	// Save completely redone graph
	r = world->store()->find(raul::Path("/"));
	const std::string redo_name = base.substr(0, base.find('.')) + ".redo.ingen";
	const FilePath    redo_path = filesystem::current_path() / redo_name;
	world->serialiser()->write_bundle(r->second, URI(redo_path));

	serd_env_free(env);
	serd_node_free(&cmds_file_uri);

	// Shut down
	world->engine()->deactivate();

	return EXIT_SUCCESS;
}

} // namespace
} // namespace test
} // namespace ingen

int
main(int argc, char** argv)
{
	ingen::set_bundle_path_from_code(
	    reinterpret_cast<void (*)()>(&ingen::test::ingen_try));

	return ingen::test::run(argc, argv);
}
