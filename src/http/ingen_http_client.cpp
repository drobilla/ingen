/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include "ingen/shared/Module.hpp"
#include "ingen/shared/World.hpp"

#include "HTTPClientReceiver.hpp"
#include "HTTPEngineSender.hpp"

using namespace std;
using namespace Ingen;

SharedPtr<Ingen::ServerInterface>
new_http_interface(Ingen::Shared::World*      world,
                   const std::string&         url,
                   SharedPtr<ClientInterface> respond_to)
{
	SharedPtr<Client::HTTPClientReceiver> receiver(
		new Client::HTTPClientReceiver(world, url, respond_to));
	Client::HTTPEngineSender* hes = new Client::HTTPEngineSender(
		world, url, receiver);
	hes->attach(rand(), true);
	return SharedPtr<ServerInterface>(hes);
}

struct IngenHTTPClientModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
		world->add_interface_factory("http", &new_http_interface);
	}
};

extern "C" {

Ingen::Shared::Module*
ingen_module_load()
{
	return new IngenHTTPClientModule();
}

} // extern "C"
