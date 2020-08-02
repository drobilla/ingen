/*
  This file is part of Ingen.
  Copyright 2007-2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/Configuration.hpp"
#include "ingen/Module.hpp"
#include "ingen/QueuedInterface.hpp"
#include "ingen/client/SigClientInterface.hpp"

#include "App.hpp"

#include <memory>

namespace ingen {
namespace gui {

struct GUIModule : public Module {
	using SigClientInterface = client::SigClientInterface;

	void load(World& world) override {
		URI uri(world.conf().option("connect").ptr<char>());
		if (!world.interface()) {
			world.set_interface(
				world.new_interface(URI(uri), make_client(world)));
		} else if (!std::dynamic_pointer_cast<SigClientInterface>(
			           world.interface()->respondee())) {
			world.interface()->set_respondee(make_client(world));
		}

		app = gui::App::create(world);
	}

	void run(World& world) override {
		app->run();
	}

	std::shared_ptr<Interface> make_client(World& world)
	{
		auto sci = std::make_shared<SigClientInterface>();
		return world.engine()
		           ? sci
		           : std::shared_ptr<Interface>(new QueuedInterface(sci));
	}

	std::shared_ptr<gui::App> app;
};

} // namespace gui
} // namespace ingen

extern "C" {

ingen::Module*
ingen_module_load()
{
	Glib::thread_init();
	return new ingen::gui::GUIModule();
}

} // extern "C"
