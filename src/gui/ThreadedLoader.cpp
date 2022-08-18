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

#include "ThreadedLoader.hpp"

#include "App.hpp"

#include "ingen/Log.hpp"
#include "ingen/Parser.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Serialiser.hpp"
#include "ingen/URI.hpp"
#include "ingen/World.hpp"
#include "ingen/client/GraphModel.hpp"
#include "raul/Path.hpp"
#include "raul/Symbol.hpp"

#include <boost/optional/optional.hpp>
#include <glibmm/ustring.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/adaptors/retype_return.h>
#include <sigc++/functors/mem_fun.h>

#include <cassert>
#include <memory>
#include <string>
#include <utility>

using boost::optional;

namespace ingen {

class Interface;

namespace gui {

ThreadedLoader::ThreadedLoader(App& app, std::shared_ptr<Interface> engine)
    : _app(app)
    , _engine(std::move(engine))
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
	if (_thread.joinable()) {
		_thread.join();
	}
}

std::shared_ptr<Parser>
ThreadedLoader::parser()
{
	return _app.world().parser();
}

void
ThreadedLoader::run()
{
	while (_sem.wait() && !_exit_flag) {
		std::lock_guard<std::mutex> lock(_mutex);
		while (!_events.empty()) {
			_events.front()();
			_events.pop_front();
		}
	}
}

void
ThreadedLoader::load_graph(bool                          merge,
                           const FilePath&               file_path,
                           const optional<raul::Path>&   engine_parent,
                           const optional<raul::Symbol>& engine_symbol,
                           const optional<Properties>&   engine_data)
{
	std::lock_guard<std::mutex> lock(_mutex);

	Glib::ustring engine_base = "";
	if (engine_parent) {
		if (merge) {
			engine_base = engine_parent.get();
		} else {
			engine_base = engine_parent.get().base();
		}
	}

	_events.emplace_back(sigc::hide_return(
	        sigc::bind(sigc::mem_fun(this, &ThreadedLoader::load_graph_event),
	                   file_path,
	                   engine_parent,
	                   engine_symbol,
	                   engine_data)));

	_sem.post();
}

void
ThreadedLoader::load_graph_event(const FilePath&               file_path,
                                 const optional<raul::Path>&   engine_parent,
                                 const optional<raul::Symbol>& engine_symbol,
                                 const optional<Properties>&   engine_data)
{
	std::lock_guard<std::mutex> lock(_app.world().rdf_mutex());

	_app.world().parser()->parse_file(_app.world(),
	                                  *_app.world().interface(),
	                                  file_path,
	                                  engine_parent,
	                                  engine_symbol,
	                                  engine_data);
}

void
ThreadedLoader::save_graph(
    const std::shared_ptr<const client::GraphModel>& model,
    const URI&                                       uri)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_events.emplace_back(sigc::hide_return(
	        sigc::bind(sigc::mem_fun(this, &ThreadedLoader::save_graph_event),
	                   model,
	                   uri)));

	_sem.post();
}

void
ThreadedLoader::save_graph_event(
    const std::shared_ptr<const client::GraphModel>& model,
    const URI&                                       uri)
{
	assert(uri.scheme() == "file");
	if (_app.serialiser()) {
		std::lock_guard<std::mutex> lock(_app.world().rdf_mutex());

		if (uri.string().find(".ingen") != std::string::npos) {
			_app.serialiser()->write_bundle(model, uri);
		} else {
			_app.serialiser()->start_to_file(model->path(), uri.file_path());
			_app.serialiser()->serialise(model);
			_app.serialiser()->finish();
		}
	}
}

} // namespace gui
} // namespace ingen
