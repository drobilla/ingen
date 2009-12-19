/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "module/Module.hpp"
#include "module/World.hpp"
#ifdef HAVE_LIBLO
#include "OSCEngineSender.hpp"
#endif
#ifdef HAVE_SOUP
#include "HTTPEngineSender.hpp"
#endif

using namespace Ingen;

#ifdef HAVE_LIBLO
SharedPtr<Ingen::Shared::EngineInterface>
new_osc_interface(Ingen::Shared::World* world, const std::string& url)
{
	Client::OSCEngineSender* oes = Client::OSCEngineSender::create(url);
	oes->attach(rand(), true);
	return SharedPtr<Shared::EngineInterface>(oes);
}
#endif

#ifdef HAVE_SOUP
SharedPtr<Ingen::Shared::EngineInterface>
new_http_interface(Ingen::Shared::World* world, const std::string& url)
{
	Client::HTTPEngineSender* hes = new Client::HTTPEngineSender(world, url);
	hes->attach(rand(), true);
	return SharedPtr<Shared::EngineInterface>(hes);
}
#endif

struct IngenClientModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
		world->interface_factories.insert(make_pair("osc.udp", &new_osc_interface));
		world->interface_factories.insert(make_pair("osc.tcp", &new_osc_interface));
		world->interface_factories.insert(make_pair("http",    &new_http_interface));
	}
};

static IngenClientModule* module = NULL;

extern "C" {

Ingen::Shared::Module*
ingen_module_load() {
	if (!module)
		module = new IngenClientModule();

	return module;
}

} // extern "C"

