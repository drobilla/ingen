/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include <string>
#include "module/global.hpp"
#include "module/World.hpp"
#include "module/Module.hpp"
#include "client/PatchModel.hpp"
#include "App.hpp"
#include "ThreadedLoader.hpp"

using namespace std;

namespace Ingen {
namespace GUI {


ThreadedLoader::ThreadedLoader(SharedPtr<EngineInterface> engine)
	: _engine(engine)
	, _deprecated_loader(engine)
{
	set_name("Loader");
	
	if (parser())
		start();
	else
		cerr << "WARNING: Failed to load ingen_serialisation module, load disabled." << endl;
}


SharedPtr<Parser>
ThreadedLoader::parser()
{
	if (_parser)
		return _parser;

	World* world = App::instance().world();
	if (!world->serialisation_module)
		world->serialisation_module = Ingen::Shared::load_module("ingen_serialisation");

	if (world->serialisation_module) {
		Parser* (*new_parser)() = NULL;

		bool found = App::instance().world()->serialisation_module->get_symbol(
				"new_parser", (void*&)new_parser);

		if (found)
			_parser = SharedPtr<Parser>(new_parser());
	}

	return _parser;
}


void
ThreadedLoader::_whipped()
{
	_mutex.lock();
	
	while ( ! _events.empty() ) {
		_events.front()();
		_events.pop_front();
	}

	_mutex.unlock();
}

void
ThreadedLoader::load_patch(bool                    merge,
                           const Glib::ustring&    data_base_uri,
                           const Path&             data_path,
                           GraphObject::Variables  engine_data,
                           optional<Path>          engine_parent,
                           optional<Symbol>        engine_symbol)
{
	_mutex.lock();

	// FIXME: Filthy hack to load deprecated patches based on file extension
	if (data_base_uri.substr(data_base_uri.length()-3) == ".om") {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(_deprecated_loader, &DeprecatedLoader::load_patch),
				data_base_uri,
				engine_parent,
				(engine_symbol) ? engine_symbol.get() : "",
				engine_data,
				false)));
	} else {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(_parser.get(), &Ingen::Serialisation::Parser::parse_document),
				App::instance().world(),
				App::instance().world()->engine.get(),
				data_base_uri, // document
				data_base_uri, // patch (root of document)
				engine_parent,
				engine_symbol,
				engine_data)));
	}
	
	whip();
	
	_mutex.unlock();
}


void
ThreadedLoader::save_patch(SharedPtr<PatchModel> model, const string& filename)
{
	_mutex.lock();

	_events.push_back(sigc::hide_return(sigc::bind(
		sigc::mem_fun(this, &ThreadedLoader::save_patch_event),
		model, filename)));

	_mutex.unlock();
	
	whip();
}


void
ThreadedLoader::save_patch_event(SharedPtr<PatchModel> model, const string& filename)
{
	if (App::instance().serialiser())
		App::instance().serialiser()->to_file(model, filename);
}


} // namespace GUI
} // namespace Ingen
