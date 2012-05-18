/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

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

#include <boost/optional.hpp>

#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/thread.h>
#include <glibmm/timer.h>

#include "raul/Configuration.hpp"
#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"
#include "raul/log.hpp"

#include "serd/serd.h"
#include "sord/sordmm.hpp"

#include "ingen_config.h"

#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/serialisation/Parser.hpp"
#include "ingen/shared/Configuration.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/runtime_paths.hpp"
#include "ingen/client/ThreadedSigClientInterface.hpp"
#ifdef WITH_BINDINGS
#include "bindings/ingen_bindings.hpp"
#endif

using namespace std;
using namespace Ingen;

Ingen::Shared::World* world = NULL;

void
ingen_try(bool cond, const char* msg)
{
	if (!cond) {
		cerr << "ingen: Error: " << msg << endl;
		delete world;
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char** argv)
{
	Glib::thread_init();
	Shared::set_bundle_path_from_code((void*)&main);

	// Create world
	try {
		world = new Ingen::Shared::World(argc, argv, NULL, NULL);
		if (argc <= 1) {
			world->conf().print_usage("ingen", cout);
			return EXIT_FAILURE;
		} else if (world->conf().option("help").get_bool()) {
			world->conf().print_usage("ingen", cout);
			return EXIT_SUCCESS;
		}
	} catch (std::exception& e) {
		cout << "ingen: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// Run engine
	//SharedPtr<Interface> engine_interface;
	ingen_try(world->load_module("server_profiled"),
	          "Unable to load server module");

	//ingen_try(world->load_module("client"),
	//        "Unable to load client module");
	
	ingen_try(world->engine(),
	          "Unable to create engine");
	
	ingen_try(world->load_module("serialisation_profiled"),
	          "Unable to load serialisation module");

	world->engine()->init(48000.0, 4096);
	world->engine()->activate();

	world->parser()->parse_file(world, world->interface().get(), argv[1]);
	while (world->engine()->run(4096)) {
		world->engine()->main_iteration();
	}

	// Shut down
	if (world->engine())
		world->engine()->deactivate();

	delete world;
	return 0;
}

