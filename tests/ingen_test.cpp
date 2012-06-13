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
#include "sratom/sratom.h"

#include "ingen_config.h"

#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/serialisation/Parser.hpp"
#include "ingen/shared/AtomReader.hpp"
#include "ingen/shared/AtomWriter.hpp"
#include "ingen/shared/Configuration.hpp"
#include "ingen/shared/URIMap.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/runtime_paths.hpp"
#include "ingen/client/ThreadedSigClientInterface.hpp"
#ifdef WITH_BINDINGS
#include "bindings/ingen_bindings.hpp"
#endif

using namespace std;
using namespace Ingen;

Shared::World* world = NULL;

/*
class TestClient : public Shared::AtomSink {
	void write(const LV2_Atom* msg) {
	}
};
*/
class TestClient : public Interface
{
public:
	TestClient() {}
	~TestClient() {}

	Raul::URI uri() const { return "http://drobilla.net/ns/ingen#AtomWriter"; }

	void bundle_begin() {}

	void bundle_end() {}

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx = Resource::DEFAULT) {}

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add) {}

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path) {}

	void del(const Raul::URI& uri) {}

	void connect(const Raul::Path& tail,
	             const Raul::Path& head) {}

	void disconnect(const Raul::Path& tail,
	                const Raul::Path& head) {}

	void disconnect_all(const Raul::Path& parent_patch_path,
	                    const Raul::Path& path) {}

	void set_property(const Raul::URI&  subject,
	                  const Raul::URI&  predicate,
	                  const Raul::Atom& value) {}

	void set_response_id(int32_t id) {}

	void get(const Raul::URI& uri) {}

	void response(int32_t id, Status status) {
		if (status) {
			Raul::error(Raul::fmt("error on message %1%: %2%\n")
			            % id % ingen_status_string(status));
			exit(EXIT_FAILURE);
		}
	}

	void error(const std::string& msg) {
		Raul::error(Raul::fmt("error: %1%\n") % msg);
		exit(EXIT_FAILURE);
	}
};


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

	if (argc != 3) {
		cerr << "Usage: ingen_test START_PATCH COMMANDS_FILE" << endl;
		return EXIT_FAILURE;
	}

	// Create world
	try {
		world = new Shared::World(argc, argv, NULL, NULL);
	} catch (std::exception& e) {
		cout << "ingen: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// Load modules
	ingen_try(world->load_module("server_profiled"),
	          "Unable to load server module");

	ingen_try(world->load_module("serialisation_profiled"),
	          "Unable to load serialisation module");

	// Initialise engine
	ingen_try(world->engine(),
	          "Unable to create engine");
	world->engine()->init(48000.0, 4096);
	world->engine()->activate();

	// Load patch
	world->parser()->parse_file(world, world->interface().get(), argv[1]);
	while (world->engine()->pending_events()) {
		world->engine()->run(4096);
		world->engine()->main_iteration();
		g_usleep(1000);
	}

	// Read commands

	SerdChunk     out    = { NULL, 0 };
	LV2_URID_Map* map    = &world->uri_map().urid_map_feature()->urid_map;
	Sratom*       sratom = sratom_new(map);

	sratom_set_object_mode(sratom, SRATOM_OBJECT_MODE_BLANK_SUBJECT);

	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, map);
	lv2_atom_forge_set_sink(&forge, sratom_forge_sink, sratom_forge_deref, &out);

	const std::string cmds_file_path = argv[2];

	// AtomReader to read commands from a file and send them to engine
	Shared::AtomReader atom_reader(
		world->uri_map(), world->uris(), world->forge(), *world->interface().get());

	// AtomWriter to serialise responses from the engine
	/*
	TestClient client;
	SharedPtr<Shared::AtomWriter> atom_writer(
		new Shared::AtomWriter(world->uri_map(), world->uris(), client));
	*/
	SharedPtr<Interface> client(new TestClient());

	world->interface()->set_respondee(client);
	world->engine()->register_client(
		"http://drobilla.net/ns/ingen#AtomWriter",
		client);

	SerdURI cmds_base;
	const SerdNode cmds_file_uri = serd_node_new_file_uri(
		(const uint8_t*)cmds_file_path.c_str(),
		NULL, &cmds_base, false);
	Sord::Model* cmds = new Sord::Model(*world->rdf_world(),
	                                    (const char*)cmds_file_uri.buf);
	SerdEnv* env = serd_env_new(&cmds_file_uri);
	cmds->load_file(env, SERD_TURTLE, cmds_file_path);
	Sord::Node nil;
	for (int i = 0; ; ++i) {
		std::string subject_str = (Raul::fmt("msg%1%") % i).str();
		Sord::URI subject(*world->rdf_world(), subject_str,
		                  (const char*)cmds_file_uri.buf);
		Sord::Iter iter = cmds->find(subject, nil, nil);
		if (iter.end()) {
			break;
		}

		out.len = 0;
		sratom_read(sratom, &forge, world->rdf_world()->c_obj(),
		            cmds->c_obj(), subject.c_obj());

		/*
		cerr << "READ " << out.len << " BYTES" << endl;
		LV2_Atom* atom = (LV2_Atom*)out.buf;
		cerr << sratom_to_turtle(
			sratom,
			&world->uri_map().urid_unmap_feature()->urid_unmap,
			(const char*)cmds_file_uri.buf,
			NULL, NULL, atom->type, atom->size, LV2_ATOM_BODY(atom)) << endl;
		*/

		if (!atom_reader.write((LV2_Atom*)out.buf)) {
			return EXIT_FAILURE;
		}

		while (world->engine()->pending_events()) {
			world->engine()->run(4096);
			world->engine()->main_iteration();
			g_usleep(1000);
		}
	}
	serd_env_free(env);
	delete cmds;

	// Shut down
	world->engine()->deactivate();

	delete world;
	return 0;
}
