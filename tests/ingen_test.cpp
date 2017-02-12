/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "raul/Path.hpp"

#include "serd/serd.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

#include "ingen_config.h"

#include "ingen/AtomReader.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/Configuration.hpp"
#include "ingen/EngineBase.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/Store.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ThreadedSigClientInterface.hpp"
#include "ingen/runtime_paths.hpp"
#include "ingen/types.hpp"

using namespace std;
using namespace Ingen;

World* world = NULL;

class TestClient : public Interface
{
public:
	explicit TestClient(Log& log) : _log(log) {}
	~TestClient() {}

	Raul::URI uri() const { return Raul::URI("ingen:testClient"); }

	void bundle_begin() {}

	void bundle_end() {}

	void put(const Raul::URI&            uri,
	         const Resource::Properties& properties,
	         Resource::Graph             ctx = Resource::Graph::DEFAULT) {}

	void delta(const Raul::URI&            uri,
	           const Resource::Properties& remove,
	           const Resource::Properties& add) {}

	void copy(const Raul::URI& old_uri,
	          const Raul::URI& new_uri) {}

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path) {}

	void del(const Raul::URI& uri) {}

	void connect(const Raul::Path& tail,
	             const Raul::Path& head) {}

	void disconnect(const Raul::Path& tail,
	                const Raul::Path& head) {}

	void disconnect_all(const Raul::Path& parent_patch_path,
	                    const Raul::Path& path) {}

	void set_property(const Raul::URI& subject,
	                  const Raul::URI& predicate,
	                  const Atom&      value) {}

	void set_response_id(int32_t id) {}

	void get(const Raul::URI& uri) {}

	void undo() {}

	void redo() {}

	void response(int32_t id, Status status, const std::string& subject) {
		if (status != Status::SUCCESS) {
			_log.error(fmt("error on message %1%: %2% (%3%)\n")
			           % id % ingen_status_string(status) % subject);
			exit(EXIT_FAILURE);
		}
	}

	void error(const std::string& msg) {
		_log.error(fmt("error: %1%\n") % msg);
		exit(EXIT_FAILURE);
	}

private:
	Log& _log;
};


static void
ingen_try(bool cond, const char* msg)
{
	if (!cond) {
		cerr << "ingen: Error: " << msg << endl;
		delete world;
		exit(EXIT_FAILURE);
	}
}

static uint32_t
flush_events(Ingen::World* world, const uint32_t start)
{
	static const uint32_t block_length = 4096;
	int                   count        = 0;
	uint32_t              offset       = start;
	while (world->engine()->pending_events()) {
		world->engine()->locate(offset, block_length);
		world->engine()->run(block_length);
		world->engine()->main_iteration();
		g_usleep(1000);
		++count;
		offset += block_length;
	}
	return offset;
}

int
main(int argc, char** argv)
{
	Glib::thread_init();
	set_bundle_path_from_code((void*)&main);

	// Create world
	try {
		world = new World(argc, argv, NULL, NULL, NULL);
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
	const char* load_path        = (const char*)load.get_body();
	char*       real_start_graph = realpath(load_path, NULL);
	if (!real_start_graph) {
		cerr << "error: initial graph '" << load_path << "' does not exist" << endl;
		return EXIT_FAILURE;
	}

	const std::string start_graph    = real_start_graph;
	const std::string cmds_file_path = (const char*)execute.get_body();
	free(real_start_graph);

	// Load modules
	ingen_try(world->load_module("server_profiled"),
	          "Unable to load server module");

	// Initialise engine
	ingen_try(bool(world->engine()),
	          "Unable to create engine");
	world->engine()->init(48000.0, 4096, 4096);
	world->engine()->activate();

	// Load patch
	if (!world->parser()->parse_file(world, world->interface().get(), start_graph)) {
		cerr << "error: failed to load initial graph " << start_graph << endl;
		return EXIT_FAILURE;
	}
	uint32_t time = flush_events(world, 0);

	// Read commands

	SerdChunk     out    = { NULL, 0 };
	LV2_URID_Map* map    = &world->uri_map().urid_map_feature()->urid_map;
	Sratom*       sratom = sratom_new(map);

	sratom_set_object_mode(sratom, SRATOM_OBJECT_MODE_BLANK_SUBJECT);

	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, map);
	lv2_atom_forge_set_sink(&forge, sratom_forge_sink, sratom_forge_deref, &out);

	// AtomReader to read commands from a file and send them to engine
	AtomReader atom_reader(world->uri_map(),
	                       world->uris(),
	                       world->log(),
	                       *world->interface().get());

	// AtomWriter to serialise responses from the engine
	SPtr<Interface> client(new TestClient(world->log()));

	world->interface()->set_respondee(client);
	world->engine()->register_client(client);

	SerdURI cmds_base;
	SerdNode cmds_file_uri = serd_node_new_file_uri(
		(const uint8_t*)cmds_file_path.c_str(),
		NULL, &cmds_base, true);
	Sord::Model* cmds = new Sord::Model(*world->rdf_world(),
	                                    (const char*)cmds_file_uri.buf);
	SerdEnv* env = serd_env_new(&cmds_file_uri);
	cmds->load_file(env, SERD_TURTLE, cmds_file_path);
	Sord::Node nil;
	int n_events = 0;
	for (;; ++n_events) {
		std::string subject_str = (fmt("msg%1%") % n_events).str();
		Sord::URI subject(*world->rdf_world(), subject_str,
		                  (const char*)cmds_file_uri.buf);
		Sord::Iter iter = cmds->find(subject, nil, nil);
		if (iter.end()) {
			break;
		}

		out.len = 0;
		sratom_read(sratom, &forge, world->rdf_world()->c_obj(),
		            cmds->c_obj(), subject.c_obj());

#if 0
		cerr << "READ " << out.len << " BYTES" << endl;
		const LV2_Atom* atom = (const LV2_Atom*)out.buf;
		cerr << sratom_to_turtle(
			sratom,
			&world->uri_map().urid_unmap_feature()->urid_unmap,
			(const char*)cmds_file_uri.buf,
			NULL, NULL, atom->type, atom->size, LV2_ATOM_BODY(atom)) << endl;
#endif

		if (!atom_reader.write((const LV2_Atom*)out.buf, n_events + 1)) {
			return EXIT_FAILURE;
		}

		time = flush_events(world, time);
	}

	delete cmds;

	// Save resulting graph
	Store::iterator   r        = world->store()->find(Raul::Path("/"));
	const std::string base     = Glib::path_get_basename(cmds_file_path);
	const std::string out_name = base.substr(0, base.find('.')) + ".out.ingen";
	const std::string out_path = Glib::build_filename(Glib::get_current_dir(), out_name);
	world->serialiser()->write_bundle(r->second, Glib::filename_to_uri(out_path));

	// Undo every event (should result in a graph identical to the original)
	for (int i = 0; i < n_events; ++i) {
		world->interface()->undo();
		time = flush_events(world, time);
	}

	// Save completely undone graph
	r = world->store()->find(Raul::Path("/"));
	const std::string undo_name = base.substr(0, base.find('.')) + ".undo.ingen";
	const std::string undo_path = Glib::build_filename(Glib::get_current_dir(), undo_name);
	world->serialiser()->write_bundle(r->second, Glib::filename_to_uri(undo_path));

	// Redo every event (should result in a graph identical to the pre-undo output)
	for (int i = 0; i < n_events; ++i) {
		world->interface()->redo();
		time = flush_events(world, time);
	}

	// Save completely redone graph
	r = world->store()->find(Raul::Path("/"));
	const std::string redo_name = base.substr(0, base.find('.')) + ".redo.ingen";
	const std::string redo_path = Glib::build_filename(Glib::get_current_dir(), redo_name);
	world->serialiser()->write_bundle(r->second, Glib::filename_to_uri(redo_path));

	free((void*)out.buf);
	serd_env_free(env);
	sratom_free(sratom);
	serd_node_free(&cmds_file_uri);

	// Shut down
	world->engine()->deactivate();

	delete world;
	return EXIT_SUCCESS;
}
