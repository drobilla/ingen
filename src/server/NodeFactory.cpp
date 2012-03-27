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


#include <float.h>
#include <math.h>
#include <stdlib.h>

#include <string>

#include <glibmm/miscutils.h>

#include "lilv/lilv.h"
#include "raul/log.hpp"
#include "sord/sordmm.hpp"

#include "internals/Controller.hpp"
#include "internals/Delay.hpp"
#include "internals/Note.hpp"
#include "internals/Trigger.hpp"
#include "ingen/shared/World.hpp"

#include "Engine.hpp"
#include "InternalPlugin.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"
#include "NodeFactory.hpp"
#include "PatchImpl.hpp"
#include "ThreadManager.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

using namespace Internals;

NodeFactory::NodeFactory(Ingen::Shared::World* world)
	: _world(world)
	, _has_loaded(false)
	, _lv2_info(new LV2Info(world))
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
		load_lv2_plugins();
		_has_loaded = true;
	}
}

void
NodeFactory::load_internal_plugins()
{
	Ingen::Shared::URIs& uris = *_world->uris().get();
	InternalPlugin* controller_plug = ControllerNode::internal_plugin(uris);
	_plugins.insert(make_pair(controller_plug->uri(), controller_plug));

	InternalPlugin* delay_plug = DelayNode::internal_plugin(uris);
	_plugins.insert(make_pair(delay_plug->uri(), delay_plug));

	InternalPlugin* note_plug = NoteNode::internal_plugin(uris);
	_plugins.insert(make_pair(note_plug->uri(), note_plug));

	InternalPlugin* trigger_plug = TriggerNode::internal_plugin(uris);
	_plugins.insert(make_pair(trigger_plug->uri(), trigger_plug));
}

/** Loads information about all LV2 plugins into internal plugin database.
 */
void
NodeFactory::load_lv2_plugins()
{
	const LilvPlugins* plugins = lilv_world_get_all_plugins(_world->lilv_world());

	LILV_FOREACH(plugins, i, plugins) {
		const LilvPlugin* lv2_plug = lilv_plugins_get(plugins, i);

		const string uri(lilv_node_as_uri(lilv_plugin_get_uri(lv2_plug)));

		assert(_plugins.find(uri) == _plugins.end());

		LV2Plugin* const plugin = new LV2Plugin(_lv2_info, uri);

		plugin->lilv_plugin(lv2_plug);
		_plugins.insert(make_pair(uri, plugin));
	}
}

} // namespace Server
} // namespace Ingen
