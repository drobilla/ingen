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

#include <cstdlib>
#include <string>
#include <pthread.h>
#include <dirent.h>
#include <float.h>
#include <cmath>
#include <glibmm/miscutils.h>
#include "sord/sordmm.hpp"
#include "raul/log.hpp"
#include "ingen-config.h"
#include "shared/World.hpp"
#include "internals/Controller.hpp"
#include "internals/Delay.hpp"
#include "internals/Note.hpp"
#include "internals/Trigger.hpp"
#include "Engine.hpp"
#include "InternalPlugin.hpp"
#include "NodeFactory.hpp"
#include "PatchImpl.hpp"
#include "ThreadManager.hpp"
#ifdef HAVE_SLV2
#include "slv2/slv2.h"
#include "LV2Plugin.hpp"
#include "LV2Node.hpp"
#endif

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

using namespace Internals;

NodeFactory::NodeFactory(Ingen::Shared::World* world)
	: _world(world)
	, _has_loaded(false)
#ifdef HAVE_SLV2
	, _lv2_info(new LV2Info(world))
#endif
{
}

NodeFactory::~NodeFactory()
{
	for (Plugins::iterator i = _plugins.begin(); i != _plugins.end(); ++i)
		delete i->second;

	_plugins.clear();
}

const NodeFactory::Plugins&
NodeFactory::plugins()
{
	if (!_has_loaded) {
		// TODO: Plugin list refreshing
		load_plugins();
	}
	return _plugins;
}

PluginImpl*
NodeFactory::plugin(const Raul::URI& uri)
{
	const Plugins::const_iterator i = _plugins.find(uri);
	return ((i != _plugins.end()) ? i->second : NULL);
}

void
NodeFactory::load_plugins()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);

	// Only load if we havn't already, so every client connecting doesn't cause
	// this (expensive!) stuff to happen.  Not the best solution - would be nice
	// if clients could refresh plugins list for whatever reason :/
	if (!_has_loaded) {
		_plugins.clear(); // FIXME: assert empty?

		load_internal_plugins();

#ifdef HAVE_SLV2
		load_lv2_plugins();
#endif

		_has_loaded = true;
	}
}

void
NodeFactory::load_internal_plugins()
{
	Ingen::Shared::LV2URIMap& uris = *_world->uris().get();
	InternalPlugin* controller_plug = ControllerNode::internal_plugin(uris);
	_plugins.insert(make_pair(controller_plug->uri(), controller_plug));

	InternalPlugin* delay_plug = DelayNode::internal_plugin(uris);
	_plugins.insert(make_pair(delay_plug->uri(), delay_plug));

	InternalPlugin* note_plug = NoteNode::internal_plugin(uris);
	_plugins.insert(make_pair(note_plug->uri(), note_plug));

	InternalPlugin* trigger_plug = TriggerNode::internal_plugin(uris);
	_plugins.insert(make_pair(trigger_plug->uri(), trigger_plug));
}

#ifdef HAVE_SLV2
/** Loads information about all LV2 plugins into internal plugin database.
 */
void
NodeFactory::load_lv2_plugins()
{
	SLV2Plugins plugins = slv2_world_get_all_plugins(_world->slv2_world());

	SLV2_FOREACH(plugins, i, plugins) {
		SLV2Plugin lv2_plug = slv2_plugins_get(plugins, i);

		const string uri(slv2_value_as_uri(slv2_plugin_get_uri(lv2_plug)));

		assert(_plugins.find(uri) == _plugins.end());

		LV2Plugin* const plugin = new LV2Plugin(_lv2_info, uri);

		plugin->slv2_plugin(lv2_plug);
		_plugins.insert(make_pair(uri, plugin));
	}
}
#endif // HAVE_SLV2

} // namespace Server
} // namespace Ingen
