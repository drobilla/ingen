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

#include "ingen_config.h"

#include "ingen/Atom.hpp"
#include "ingen/AtomForge.hpp"
#include "ingen/AtomReader.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/FilePath.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/Store.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "ingen/filesystem.hpp"
#include "ingen/fmt.hpp"
#include "ingen/runtime_paths.hpp"
#include "ingen/types.hpp"
#include "raul/Path.hpp"
#include "serd/serd.hpp"
#include "sratom/sratom.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string>
#include <utility>

using namespace std;
using namespace ingen;

unique_ptr<World> world;

static void
ingen_try(bool cond, const char* msg)
{
	if (!cond) {
		cerr << "ingen: Error: " << msg << endl;
		world.reset();
		exit(EXIT_FAILURE);
	}
}

static FilePath
real_file_path(const char* path)
{
	UPtr<char, FreeDeleter<char>> real_path{realpath(path, nullptr)};

	return FilePath{real_path.get()};
}

int
main(int argc, char** argv)
{
	set_bundle_path_from_code((void*)&ingen_try);

	// Create world
	try {
		world = unique_ptr<World>{new World(nullptr, nullptr, nullptr)};
		world->load_configuration(argc, argv);
	} catch (std::exception& e) {
		cout << "ingen: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// Get mandatory command line arguments
	const Atom& load    = world->conf().option("load");
	const Atom& execute = world->conf().option("execute");
	if (!load.is_valid() || !execute.is_valid()) {
		cerr << "Usage: ingen_test --load START_GRAPH --execute COMMANDS_FILE" << endl;
		return EXIT_FAILURE;
	}

	// Get start graph and commands file options
	const FilePath load_path = real_file_path((const char*)load.get_body());
	const FilePath run_path  = real_file_path((const char*)execute.get_body());

	if (load_path.empty()) {
		cerr << "error: initial graph '" << load_path << "' does not exist" << endl;
		return EXIT_FAILURE;
	} else if (run_path.empty()) {
		cerr << "error: command file '" << run_path << "' does not exist" << endl;
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
		cerr << "error: failed to load initial graph " << load_path << endl;
		return EXIT_FAILURE;
	}
	world->engine()->flush_events(std::chrono::milliseconds(20));

	// Read commands

	AtomForge forge(world->rdf_world(),
	                world->uri_map().urid_map_feature()->urid_map);

	// AtomReader to read commands from a file and send them to engine
	AtomReader atom_reader(world->uri_map(),
	                       world->uris(),
	                       world->log(),
	                       *world->interface().get());

	// AtomWriter to serialise responses from the engine
	SPtr<Interface> client(new TestClient(world->log()));

	world->interface()->set_respondee(client);
	world->engine()->register_client(client);

	serd::Node  run_uri = serd::make_file_uri(run_path.string());
	serd::Model cmds(world->rdf_world(),
	                 serd::ModelFlag::index_SPO | serd::ModelFlag::index_OPS);

	serd::Env      env(run_uri);
	serd::Inserter inserter(cmds, env);
	serd::Reader   reader(
		world->rdf_world(), serd::Syntax::Turtle, {}, inserter.sink(), 4096);

	reader.start_file(run_path.string());
	reader.read_document();
	reader.finish();

	int n_events = 0;
	for (;; ++n_events) {
		const auto subject = serd::make_resolved_uri(
			std::string("msg") + std::to_string(n_events), run_uri);
		if (!cmds.ask(subject, {}, {})) {
			break;
		}

		auto atom = forge.read(cmds, subject);

#if 0
		sratom::Streamer streamer{
		        world->rdf_world(),
		        world->uri_map().urid_map_feature()->urid_map,
		        world->uri_map().urid_unmap_feature()->urid_unmap};

		auto str = streamer.to_string(env, *atom);
		cerr << "Read " << atom->size << " bytes:\n" << str << endl;
#endif

		if (!atom_reader.write(atom, n_events + 1)) {
			return EXIT_FAILURE;
		}

		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	// Save resulting graph
	auto              r        = world->store()->find(Raul::Path("/"));
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
	r = world->store()->find(Raul::Path("/"));
	const std::string undo_name = base.substr(0, base.find('.')) + ".undo.ingen";
	const FilePath    undo_path = filesystem::current_path() / undo_name;
	world->serialiser()->write_bundle(r->second, URI(undo_path));

	// Redo every event (should result in a graph identical to the pre-undo output)
	for (int i = 0; i < n_events; ++i) {
		world->interface()->redo();
		world->engine()->flush_events(std::chrono::milliseconds(20));
	}

	// Save completely redone graph
	r = world->store()->find(Raul::Path("/"));
	const std::string redo_name = base.substr(0, base.find('.')) + ".redo.ingen";
	const FilePath    redo_path = filesystem::current_path() / redo_name;
	world->serialiser()->write_bundle(r->second, URI(redo_path));

	// Shut down
	world->engine()->deactivate();

	return EXIT_SUCCESS;
}
