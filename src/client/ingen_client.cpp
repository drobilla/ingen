/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but HAVEOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "ingen-config.h"
#include "raul/SharedPtr.hpp"
#include "shared/Module.hpp"
#include "shared/World.hpp"
#ifdef HAVE_LIBLO
#include "OSCEngineSender.hpp"
#endif
#ifdef HAVE_SOUP
#include "HTTPEngineSender.hpp"
#endif

using namespace Ingen;

#ifdef HAVE_LIBLO
SharedPtr<Ingen::EngineInterface>
new_osc_interface(Ingen::Shared::World* world, const std::string& url)
{
	Client::OSCEngineSender* oes = Client::OSCEngineSender::create(url);
	oes->attach(rand(), true);
	return SharedPtr<EngineInterface>(oes);
}
#endif

#ifdef HAVE_SOUP
SharedPtr<Ingen::EngineInterface>
new_http_interface(Ingen::Shared::World* world, const std::string& url)
{
	Client::HTTPEngineSender* hes = new Client::HTTPEngineSender(world, url);
	hes->attach(rand(), true);
	return SharedPtr<EngineInterface>(hes);
}
#endif

struct IngenClientModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
#ifdef HAVE_LIBLO
		world->add_interface_factory("osc.udp", &new_osc_interface);
		world->add_interface_factory("osc.tcp", &new_osc_interface);
#endif
#ifdef HAVE_SOUP
		world->add_interface_factory("http",    &new_http_interface);
#endif
	}
};

extern "C" {

Ingen::Shared::Module*
ingen_module_load()
{
	return new IngenClientModule();
}

} // extern "C"

