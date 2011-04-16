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

#ifndef INGEN_ENGINE_NODEFACTORY_HPP
#define INGEN_ENGINE_NODEFACTORY_HPP

#include <list>
#include <map>
#include <string>
#include <pthread.h>
#include <glibmm/module.h>
#include "raul/SharedPtr.hpp"
#include "raul/URI.hpp"
#include "shared/World.hpp"
#include "ingen-config.h"

namespace Ingen {

class NodeImpl;
class PatchImpl;
class PluginImpl;
#ifdef HAVE_SLV2
class LV2Info;
#endif

/** Discovers and loads plugin libraries.
 *
 * \ingroup engine
 */
class NodeFactory
{
public:
	explicit NodeFactory(Ingen::Shared::World* world);
	~NodeFactory();

	void load_plugins();

	typedef std::map<Raul::URI, PluginImpl*> Plugins;
	const Plugins& plugins() const { return _plugins; }

	PluginImpl* plugin(const Raul::URI& uri);

private:
#ifdef HAVE_SLV2
	void load_lv2_plugins();
#endif

	void load_internal_plugins();

	Plugins               _plugins;
	Ingen::Shared::World* _world;
	bool                  _has_loaded;
#ifdef HAVE_SLV2
	SharedPtr<LV2Info>    _lv2_info;
#endif
};

} // namespace Ingen

#endif // INGEN_ENGINE_NODEFACTORY_HPP
