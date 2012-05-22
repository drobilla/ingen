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

#include "ingen/client/ClientStore.hpp"
#include "ingen/client/PatchModel.hpp"
#include "ingen/client/SigClientInterface.hpp"
#include "ingen/shared/AtomReader.hpp"
#include "ingen/shared/AtomSink.hpp"
#include "ingen/shared/AtomWriter.hpp"
#include "ingen/shared/Configuration.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/runtime_paths.hpp"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include "App.hpp"
#include "PatchBox.hpp"

#define INGEN_LV2_UI_URI "http://drobilla.net/ns/ingen#PatchUIGtk2"

/** A sink that writes atoms to a port via the UI extension. */
struct IngenLV2AtomSink : public Ingen::Shared::AtomSink {
	IngenLV2AtomSink(Ingen::Shared::URIs& uris,
	                 LV2UI_Write_Function ui_write,
	                 LV2UI_Controller     ui_controller)
		: _uris(uris)
		, _ui_write(ui_write)
		, _ui_controller(ui_controller)
	{}

	bool write(const LV2_Atom* atom) {
		_ui_write(_ui_controller,
		          0,
		          lv2_atom_total_size(atom),
		          _uris.atom_eventTransfer,
		          atom);
		return true;
	}

	Ingen::Shared::URIs& _uris;
	LV2UI_Write_Function _ui_write;
	LV2UI_Controller     _ui_controller;
};

struct IngenLV2UI {
	IngenLV2UI()
		: argc(0)
		, argv(NULL)
		, forge(NULL)
		, world(NULL)
		, sink(NULL)
	{}

	int                                          argc;
	char**                                       argv;
	Ingen::Shared::Forge*                        forge;
	Ingen::Shared::World*                        world;
	IngenLV2AtomSink*                            sink;
	SharedPtr<Ingen::GUI::App>                   app;
	SharedPtr<Ingen::GUI::PatchBox>              view;
	SharedPtr<Ingen::Interface>                  engine;
	SharedPtr<Ingen::Shared::AtomReader>         reader;
	SharedPtr<Ingen::Client::SigClientInterface> client;
};

static LV2UI_Handle
instantiate(const LV2UI_Descriptor*   descriptor,
            const char*               plugin_uri,
            const char*               bundle_path,
            LV2UI_Write_Function      write_function,
            LV2UI_Controller          controller,
            LV2UI_Widget*             widget,
            const LV2_Feature* const* features)
{
	Ingen::Shared::set_bundle_path(bundle_path);

	IngenLV2UI* ui = new IngenLV2UI();

	LV2_URID_Map*   map   = NULL;
	LV2_URID_Unmap* unmap = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_URID_URI "#unmap")) {
			unmap = (LV2_URID_Unmap*)features[i]->data;
		}
	}

	ui->world = new Ingen::Shared::World(
		ui->argc, ui->argv, map, unmap);

	ui->forge = new Ingen::Shared::Forge(ui->world->uri_map());

	if (!ui->world->load_module("client")) {
		delete ui;
		return NULL;
	}

	ui->sink = new IngenLV2AtomSink(
		ui->world->uris(), write_function, controller);

	// Set up an engine interface that writes LV2 atoms
	ui->engine = SharedPtr<Ingen::Interface>(
		new Ingen::Shared::AtomWriter(
			ui->world->uri_map(), ui->world->uris(), *ui->sink));

	ui->world->set_interface(ui->engine);

	// Create App and client
	ui->app = Ingen::GUI::App::create(ui->world);
	ui->client = SharedPtr<Ingen::Client::SigClientInterface>(
		new Ingen::Client::SigClientInterface());
	ui->app->attach(ui->client);

	ui->reader = SharedPtr<Ingen::Shared::AtomReader>(
		new Ingen::Shared::AtomReader(ui->world->uri_map(),
		                              ui->world->uris(),
		                              ui->world->forge(),
		                              *ui->client.get()));

	// Create empty root patch model
	Ingen::Resource::Properties props;
	props.insert(std::make_pair(ui->app->uris().rdf_type,
	                            ui->app->uris().ingen_Patch));
	ui->app->store()->put("path:/", props);

	// Create a PatchBox for the root and set as the UI widget
	SharedPtr<const Ingen::Client::PatchModel> root = PtrCast<const Ingen::Client::PatchModel>(
		ui->app->store()->object("path:/"));
	ui->view = Ingen::GUI::PatchBox::create(*ui->app, root);
	ui->view->unparent();
	*widget = ui->view->gobj();

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	IngenLV2UI* ui = (IngenLV2UI*)handle;
	delete ui;
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
	IngenLV2UI* ui   = (IngenLV2UI*)handle;
	LV2_Atom*   atom = (LV2_Atom*)buffer;
	ui->reader->write(atom);
}

const void*
extension_data(const char* uri)
{
	return NULL;
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
		return NULL;
	}
}
