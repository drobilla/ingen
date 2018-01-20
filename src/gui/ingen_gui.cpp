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
#include "ingen/client/ThreadedSigClientInterface.hpp"

#include "App.hpp"

namespace Ingen {
namespace GUI {

struct GUIModule : public Module {
	void load(World* world) {
		using Client::SigClientInterface;
		using Client::ThreadedSigClientInterface;

		std::string uri = world->conf().option("connect").ptr<char>();
		if (!world->interface()) {
			SPtr<SigClientInterface> client(new ThreadedSigClientInterface());
			world->set_interface(world->new_interface(URI(uri), client));
		} else if (!dynamic_ptr_cast<Client::SigClientInterface>(
			           world->interface()->respondee())) {
			SPtr<SigClientInterface> client(new ThreadedSigClientInterface());
			world->interface()->set_respondee(client);
		}

		app = GUI::App::create(world);
	}

	void run(World* world) {
		app->run();
	}

	SPtr<GUI::App> app;
};

} // namespace GUI
} // namespace Ingen

extern "C" {

Ingen::Module*
ingen_module_load()
{
	Glib::thread_init();
	return new Ingen::GUI::GUIModule();
}

} // extern "C"
