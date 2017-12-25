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

#include <string>

#include "ingen/Log.hpp"
#include "ingen/Module.hpp"
#include "ingen/World.hpp"
#include "ingen/client/GraphModel.hpp"

#include "App.hpp"
#include "ThreadedLoader.hpp"

using namespace boost;
using namespace std;

namespace Ingen {
namespace GUI {

ThreadedLoader::ThreadedLoader(App& app, SPtr<Interface> engine)
	: _app(app)
	, _sem(0)
	, _engine(engine)
	, _exit_flag(false)
	, _thread(&ThreadedLoader::run, this)
{
	if (!parser()) {
		app.log().warn("Parser unavailable, graph loading disabled\n");
	}
}

ThreadedLoader::~ThreadedLoader()
{
	_exit_flag = true;
	_sem.post();
	_thread.join();
}

SPtr<Parser>
ThreadedLoader::parser()
{
	return _app.world()->parser();
}

void
ThreadedLoader::run()
{
	while (_sem.wait() && !_exit_flag) {
		_mutex.lock();
		while (!_events.empty()) {
			_events.front()();
			_events.pop_front();
		}
		_mutex.unlock();
	}
}

void
ThreadedLoader::load_graph(bool                   merge,
                           const Glib::ustring&   document_uri,
                           optional<Raul::Path>   engine_parent,
                           optional<Raul::Symbol> engine_symbol,
                           optional<Properties>   engine_data)
{
	_mutex.lock();

	Glib::ustring engine_base = "";
	if (engine_parent) {
		if (merge) {
			engine_base = engine_parent.get();
		} else {
			engine_base = engine_parent.get().base();
		}
	}

	_events.push_back(
		sigc::hide_return(
			sigc::bind(sigc::mem_fun(this, &ThreadedLoader::load_graph_event),
			           document_uri,
			           engine_parent,
			           engine_symbol,
			           engine_data)));

	_mutex.unlock();
	_sem.post();
}

void
ThreadedLoader::load_graph_event(const Glib::ustring&   document_uri,
                                 optional<Raul::Path>   engine_parent,
                                 optional<Raul::Symbol> engine_symbol,
                                 optional<Properties>   engine_data)
{
	std::lock_guard<std::mutex> lock(_app.world()->rdf_mutex());

	_app.world()->parser()->parse_file(_app.world(),
	                                   _app.world()->interface().get(),
	                                   document_uri,
	                                   engine_parent,
	                                   engine_symbol,
	                                   engine_data);
}

void
ThreadedLoader::save_graph(SPtr<const Client::GraphModel> model,
                           const string&                  filename)
{
	_mutex.lock();

	_events.push_back(
		sigc::hide_return(
			sigc::bind(sigc::mem_fun(this, &ThreadedLoader::save_graph_event),
			           model, filename)));

	_mutex.unlock();
	_sem.post();
}

void
ThreadedLoader::save_graph_event(SPtr<const Client::GraphModel> model,
                                 const string&                  filename)
{
	if (_app.serialiser()) {
		std::lock_guard<std::mutex> lock(_app.world()->rdf_mutex());

		if (filename.find(".ingen") != string::npos) {
			_app.serialiser()->write_bundle(model, filename);
		} else {
			_app.serialiser()->start_to_file(model->path(), filename);
			_app.serialiser()->serialise(model);
			_app.serialiser()->finish();
		}
	}
}

} // namespace GUI
} // namespace Ingen
