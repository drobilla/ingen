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

#include <stdlib.h>

#include "lilv/lilv.h"

#include "ingen/World.hpp"
#include "internals/Controller.hpp"
#include "internals/Delay.hpp"
#include "internals/Note.hpp"
#include "internals/Time.hpp"
#include "internals/Trigger.hpp"

#include "InternalPlugin.hpp"
#include "LV2Plugin.hpp"
#include "BlockFactory.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {
namespace Server {

using namespace Internals;

BlockFactory::BlockFactory(Ingen::World* world)
	: _world(world)
	, _lv2_info(new LV2Info(world))
	, _has_loaded(false)
{
	load_internal_plugins();
}

BlockFactory::~BlockFactory()
{
	for (Plugins::iterator i = _plugins.begin(); i != _plugins.end(); ++i)
		delete i->second;

	_plugins.clear();
}

const BlockFactory::Plugins&
BlockFactory::plugins()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	if (!_has_loaded) {
		_has_loaded = true;
		// TODO: Plugin list refreshing
		load_lv2_plugins();
		_has_loaded = true;
	}
	return _plugins;
}

PluginImpl*
BlockFactory::plugin(const Raul::URI& uri)
{
	load_plugin(uri);
	const Plugins::const_iterator i = _plugins.find(uri);
	return ((i != _plugins.end()) ? i->second : NULL);
}

void
BlockFactory::load_internal_plugins()
{
	Ingen::URIs& uris = _world->uris();
	InternalPlugin* controller_plug = ControllerNode::internal_plugin(uris);
	_plugins.insert(make_pair(controller_plug->uri(), controller_plug));

	InternalPlugin* delay_plug = DelayNode::internal_plugin(uris);
	_plugins.insert(make_pair(delay_plug->uri(), delay_plug));

	InternalPlugin* note_plug = NoteNode::internal_plugin(uris);
	_plugins.insert(make_pair(note_plug->uri(), note_plug));

	InternalPlugin* time_plug = TimeNode::internal_plugin(uris);
	_plugins.insert(make_pair(time_plug->uri(), time_plug));

	InternalPlugin* trigger_plug = TriggerNode::internal_plugin(uris);
	_plugins.insert(make_pair(trigger_plug->uri(), trigger_plug));
}

void
BlockFactory::load_plugin(const Raul::URI& uri)
{
	if (_has_loaded || _plugins.find(uri) != _plugins.end()) {
		return;
	}

	LilvNode*          node  = lilv_new_uri(_world->lilv_world(), uri.c_str());
	const LilvPlugins* plugs = lilv_world_get_all_plugins(_world->lilv_world());
	const LilvPlugin*  plug  = lilv_plugins_get_by_uri(plugs, node);
	if (plug) {
		LV2Plugin* const ingen_plugin = new LV2Plugin(_lv2_info, uri);
		ingen_plugin->lilv_plugin(plug);
		_plugins.insert(make_pair(uri, ingen_plugin));
	}
	lilv_node_free(node);
}

/** Loads information about all LV2 plugins into internal plugin database.
 */
void
BlockFactory::load_lv2_plugins()
{
	const LilvPlugins* plugins = lilv_world_get_all_plugins(_world->lilv_world());
	LILV_FOREACH(plugins, i, plugins) {
		const LilvPlugin* lv2_plug = lilv_plugins_get(plugins, i);

		const Raul::URI uri(lilv_node_as_uri(lilv_plugin_get_uri(lv2_plug)));

		if (_plugins.find(uri) != _plugins.end()) {
			continue;
		}

		LV2Plugin* const plugin = new LV2Plugin(_lv2_info, uri);

		plugin->lilv_plugin(lv2_plug);
		_plugins.insert(make_pair(uri, plugin));
	}
}

} // namespace Server
} // namespace Ingen
