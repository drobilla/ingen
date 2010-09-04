/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include <string>
#include "raul/log.hpp"
#include "module/World.hpp"
#include "module/Module.hpp"
#include "module/ingen_module.hpp"
#include "App.hpp"
#include "ThreadedLoader.hpp"
#include "client/PatchModel.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace GUI {


ThreadedLoader::ThreadedLoader(SharedPtr<Shared::LV2URIMap> uris, SharedPtr<EngineInterface> engine)
	: _engine(engine)
	, _deprecated_loader(uris, engine)
{
	set_name("Loader");

	if (parser())
		start();
	else
		warn << "Failed to load ingen_serialisation module, load disabled." << endl;
}


SharedPtr<Parser>
ThreadedLoader::parser()
{
	Ingen::Shared::World* world = App::instance().world();

	if (!world->parser())
		world->load("ingen_serialisation");

	return world->parser();
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
ThreadedLoader::load_patch(bool                             merge,
                           const Glib::ustring&             document_uri,
                           optional<Path>                   data_path,
                           optional<Path>                   engine_parent,
                           optional<Symbol>                 engine_symbol,
                           optional<GraphObject::Properties> engine_data)
{
	_mutex.lock();

	Ingen::Shared::World* world = App::instance().world();

	Glib::ustring engine_base = "";
	if (engine_parent) {
		if (merge)
			engine_base = engine_parent.get().str();
		else
			engine_base = engine_parent.get().base();
	}

	// Filthy hack to load deprecated patches based on file extension
	if (document_uri.substr(document_uri.length()-3) == ".om") {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(_deprecated_loader, &DeprecatedLoader::load_patch),
				document_uri,
				merge,
				engine_parent,
				engine_symbol,
				*engine_data,
				false)));
	} else {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(world->parser().get(), &Ingen::Serialisation::Parser::parse_document),
				App::instance().world(),
				App::instance().world()->engine().get(),
				document_uri,
				data_path,
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
	if (App::instance().serialiser()) {
		Serialiser::Record r(model, filename);
		if (filename.find(".ing.lv2") != string::npos)
			App::instance().serialiser()->write_bundle(r);
		else
			App::instance().serialiser()->to_file(r);
	}
}


} // namespace GUI
} // namespace Ingen
