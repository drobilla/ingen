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
#include "ingen/Clock.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Forge.hpp"
#include "ingen/Parser.hpp"
#include "ingen/World.hpp"
#include "ingen/runtime_paths.hpp"
#include "ingen/types.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

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

static std::string
real_path(const char* path)
{
	char* const c_real_path = realpath(path, nullptr);
	const std::string result(c_real_path ? c_real_path : "");
	free(c_real_path);
	return result;
}

int
main(int argc, char** argv)
{
	set_bundle_path_from_code((void*)&ingen_try);

	// Create world
	try {
		world = unique_ptr<World>{new World(nullptr, nullptr, nullptr)};
		world->conf().add(
			"output", "output", 'O', "File to write benchmark output",
			ingen::Configuration::SESSION, world->forge().String, Atom());
		world->load_configuration(argc, argv);
	} catch (std::exception& e) {
		cout << "ingen: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// Get mandatory command line arguments
	const Atom& load = world->conf().option("load");
	const Atom& out  = world->conf().option("output");
	if (!load.is_valid() || !out.is_valid()) {
		cerr << "Usage: ingen_bench --load START_GRAPH --output OUT_FILE" << endl;
		return EXIT_FAILURE;
	}

	// Get start graph and output file options
	const std::string start_graph = real_path((const char*)load.get_body());
	const std::string out_file    = (const char*)out.get_body();
	if (start_graph.empty()) {
		cerr << "error: initial graph '"
		     << ((const char*)load.get_body())
		     << "' does not exist" << endl;
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
	if (!world->parser()->parse_file(*world, *world->interface(), start_graph)) {
		cerr << "error: failed to load initial graph " << start_graph << endl;
		return EXIT_FAILURE;
	}
	world->engine()->flush_events(std::chrono::milliseconds(20));

	// Run benchmark
	// TODO: Set up real-time scheduling for this and worker threads
	ingen::Clock   clock;
	const uint32_t n_test_frames = 1 << 20;
	const uint32_t block_length  = 4096;
	const uint64_t t_start       = clock.now_microseconds();
	for (uint32_t i = 0; i < n_test_frames; i += block_length) {
		world->engine()->advance(block_length);
		world->engine()->run(block_length);
		//world->engine()->main_iteration();
	}
	const uint64_t t_end = clock.now_microseconds();

	// Write log output
	std::unique_ptr<FILE, decltype(&fclose)> log{fopen(out_file.c_str(), "a"),
	                                             &fclose};
	if (ftell(log.get()) == 0) {
		fprintf(log.get(), "# n_threads\trun_time\treal_time\n");
	}
	fprintf(log.get(), "%u\t%f\t%f\n",
	        world->conf().option("threads").get<int32_t>(),
	        (t_end - t_start) / 1000000.0,
	        (n_test_frames / 48000.0));

	// Shut down
	world->engine()->deactivate();

	return EXIT_SUCCESS;
}
