/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/AtomReader.hpp"
#include "ingen/AtomSink.hpp"
#include "ingen/AtomWriter.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ClientStore.hpp"
#include "ingen/client/GraphModel.hpp"
#include "ingen/client/SigClientInterface.hpp"
#include "ingen/ingen.h"
#include "ingen/runtime_paths.hpp"
#include "ingen/types.hpp"
#include "lv2/ui/ui.h"

#include "App.hpp"
#include "GraphBox.hpp"

#define INGEN_LV2_UI_URI INGEN_NS "GraphUIGtk2"

namespace Ingen {

/** A sink that writes atoms to a port via the UI extension. */
struct IngenLV2AtomSink : public AtomSink {
	IngenLV2AtomSink(URIs&                uris,
	                 LV2UI_Write_Function ui_write,
	                 LV2UI_Controller     ui_controller)
		: _uris(uris)
		, _ui_write(ui_write)
		, _ui_controller(ui_controller)
	{}

	bool write(const LV2_Atom* atom, int32_t default_id) {
		_ui_write(_ui_controller,
		          0,
		          lv2_atom_total_size(atom),
		          _uris.atom_eventTransfer,
		          atom);
		return true;
	}

	URIs&                _uris;
	LV2UI_Write_Function _ui_write;
	LV2UI_Controller     _ui_controller;
};

struct IngenLV2UI {
	IngenLV2UI()
		: argc(0)
		, argv(nullptr)
		, forge(nullptr)
		, world(nullptr)
		, sink(nullptr)
	{}

	int                              argc;
	char**                           argv;
	Forge*                           forge;
	World*                           world;
	IngenLV2AtomSink*                sink;
	SPtr<GUI::App>                   app;
	SPtr<GUI::GraphBox>              view;
	SPtr<Interface>                  engine;
	SPtr<AtomReader>                 reader;
	SPtr<Client::SigClientInterface> client;
};

} // namespace Ingen

static LV2UI_Handle
instantiate(const LV2UI_Descriptor*   descriptor,
            const char*               plugin_uri,
            const char*               bundle_path,
            LV2UI_Write_Function      write_function,
            LV2UI_Controller          controller,
            LV2UI_Widget*             widget,
            const LV2_Feature* const* features)
{
#if __cplusplus >= 201103L
	using Ingen::SPtr;
#endif

	Ingen::set_bundle_path(bundle_path);

	Ingen::IngenLV2UI* ui = new Ingen::IngenLV2UI();

	LV2_URID_Map*   map   = nullptr;
	LV2_URID_Unmap* unmap = nullptr;
	LV2_Log_Log*    log   = nullptr;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_URID__unmap)) {
			unmap = (LV2_URID_Unmap*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_LOG__log)) {
			log = (LV2_Log_Log*)features[i]->data;
		}
	}

	ui->world = new Ingen::World(map, unmap, log);
	ui->forge = new Ingen::Forge(ui->world->uri_map());

	ui->world->load_configuration(ui->argc, ui->argv);

	if (!ui->world->load_module("client")) {
		delete ui;
		return nullptr;
	}

	ui->sink = new Ingen::IngenLV2AtomSink(
		ui->world->uris(), write_function, controller);

	// Set up an engine interface that writes LV2 atoms
	ui->engine = SPtr<Ingen::Interface>(
		new Ingen::AtomWriter(
			ui->world->uri_map(), ui->world->uris(), *ui->sink));

	ui->world->set_interface(ui->engine);

	// Create App and client
	ui->app = Ingen::GUI::App::create(ui->world);
	ui->client = SPtr<Ingen::Client::SigClientInterface>(
		new Ingen::Client::SigClientInterface());
	ui->app->set_is_plugin(true);
	ui->app->attach(ui->client);

	ui->reader = SPtr<Ingen::AtomReader>(
		new Ingen::AtomReader(ui->world->uri_map(),
		                      ui->world->uris(),
		                      ui->world->log(),
		                      *ui->client.get()));

	// Create empty root graph model
	Ingen::Properties props;
	props.emplace(ui->app->uris().rdf_type,
	              Ingen::Property(ui->app->uris().ingen_Graph));
	ui->app->store()->put(Ingen::main_uri(), props);

	// Create a GraphBox for the root and set as the UI widget
	SPtr<const Ingen::Client::GraphModel> root =
		Ingen::dynamic_ptr_cast<const Ingen::Client::GraphModel>(
			ui->app->store()->object(Raul::Path("/")));
	ui->view = Ingen::GUI::GraphBox::create(*ui->app, root);
	ui->view->unparent();
	*widget = ui->view->gobj();

	// Request the actual root graph
	ui->world->interface()->get(Ingen::main_uri());

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	Ingen::IngenLV2UI* ui = (Ingen::IngenLV2UI*)handle;
	delete ui;
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	Ingen::IngenLV2UI* ui   = (Ingen::IngenLV2UI*)handle;
	const LV2_Atom*    atom = (const LV2_Atom*)buffer;
	ui->reader->write(atom);
}

static const void*
extension_data(const char* uri)
{
	return nullptr;
}

static const LV2UI_Descriptor descriptor = {
	INGEN_LV2_UI_URI,
	instantiate,
	cleanup,
	port_event,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return nullptr;
	}
}
