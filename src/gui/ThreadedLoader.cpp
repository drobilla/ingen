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

#include <string>
#include "raul/log.hpp"
#include "ingen/shared/World.hpp"
#include "ingen/shared/Module.hpp"
#include "App.hpp"
#include "ThreadedLoader.hpp"
#include "ingen/client/PatchModel.hpp"

using namespace Raul;
using namespace boost;
using namespace std;

namespace Ingen {
namespace GUI {

ThreadedLoader::ThreadedLoader(App& app, SharedPtr<Interface> engine)
	: _app(app)
	, _engine(engine)
{
	set_name("Loader");

	if (parser())
		start();
	else
		warn << "Failed to load ingen_serialisation module, load disabled." << endl;
}

SharedPtr<Serialisation::Parser>
ThreadedLoader::parser()
{
	Ingen::Shared::World* world = _app.world();

	if (!world->parser())
		world->load_module("serialisation");

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
ThreadedLoader::load_patch(bool                              merge,
                           const Glib::ustring&              document_uri,
                           optional<Path>                    engine_parent,
                           optional<Symbol>                  engine_symbol,
                           optional<GraphObject::Properties> engine_data)
{
	_mutex.lock();

	Ingen::Shared::World* world = _app.world();

	Glib::ustring engine_base = "";
	if (engine_parent) {
		if (merge)
			engine_base = engine_parent.get().str();
		else
			engine_base = engine_parent.get().base();
	}

	_events.push_back(
		sigc::hide_return(
			sigc::bind(sigc::mem_fun(world->parser().get(),
			                         &Ingen::Serialisation::Parser::parse_file),
			           _app.world(),
			           _app.world()->engine().get(),
			           document_uri,
			           engine_parent,
			           engine_symbol,
			           engine_data)));

	whip();

	_mutex.unlock();
}

void
ThreadedLoader::save_patch(SharedPtr<const Client::PatchModel> model,
                           const string&                       filename)
{
	_mutex.lock();

	_events.push_back(sigc::hide_return(sigc::bind(
		sigc::mem_fun(this, &ThreadedLoader::save_patch_event),
		model, filename)));

	_mutex.unlock();

	whip();
}

void
ThreadedLoader::save_patch_event(SharedPtr<const Client::PatchModel> model,
                                 const string&                       filename)
{
	if (_app.serialiser()) {
		if (filename.find(".ingen") != string::npos)
			_app.serialiser()->write_bundle(model, filename);
		else
			_app.serialiser()->to_file(model, filename);
	}
}

} // namespace GUI
} // namespace Ingen
