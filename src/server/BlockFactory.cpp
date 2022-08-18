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

#include "BlockFactory.hpp"

#include "InternalPlugin.hpp"
#include "LV2Plugin.hpp"
#include "PluginImpl.hpp"
#include "PortType.hpp"
#include "ThreadManager.hpp"

#include "ingen/LV2Features.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIs.hpp"
#include "ingen/World.hpp"
#include "internals/BlockDelay.hpp"
#include "internals/Controller.hpp"
#include "internals/Note.hpp"
#include "internals/Time.hpp"
#include "internals/Trigger.hpp"
#include "lilv/lilv.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace ingen {
namespace server {

BlockFactory::BlockFactory(ingen::World& world)
	: _world(world)
{
	load_internal_plugins();
}

const BlockFactory::Plugins&
BlockFactory::plugins()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	if (!_has_loaded) {
		load_lv2_plugins();
		_has_loaded = true;
	}
	return _plugins;
}

std::set<std::shared_ptr<PluginImpl>>
BlockFactory::refresh()
{
	// Record current plugins, and those that are currently zombies
	const Plugins                         old_plugins(_plugins);
	std::set<std::shared_ptr<PluginImpl>> zombies;
	for (const auto& p : _plugins) {
		if (p.second->is_zombie()) {
			zombies.insert(p.second);
		}
	}

	// Re-load plugins
	load_lv2_plugins();

	// Add any new plugins to response
	std::set<std::shared_ptr<PluginImpl>> new_plugins;
	for (const auto& p : _plugins) {
		auto o = old_plugins.find(p.first);
		if (o == old_plugins.end()) {
			new_plugins.insert(p.second);
		}
	}

	// Add any resurrected plugins to response
	std::copy_if(zombies.begin(),
	             zombies.end(),
	             std::inserter(new_plugins, new_plugins.end()),
	             [](const auto& z) { return !z->is_zombie(); });

	return new_plugins;
}

PluginImpl*
BlockFactory::plugin(const URI& uri)
{
	load_plugin(uri);
	const auto i = _plugins.find(uri);
	return ((i != _plugins.end()) ? i->second.get() : nullptr);
}

void
BlockFactory::load_internal_plugins()
{
	ingen::URIs& uris = _world.uris();

	InternalPlugin* block_delay =
	    internals::BlockDelayNode::internal_plugin(uris);
	_plugins.emplace(block_delay->uri(), block_delay);

	InternalPlugin* controller =
	    internals::ControllerNode::internal_plugin(uris);
	_plugins.emplace(controller->uri(), controller);

	InternalPlugin* note = internals::NoteNode::internal_plugin(uris);
	_plugins.emplace(note->uri(), note);

	InternalPlugin* time = internals::TimeNode::internal_plugin(uris);
	_plugins.emplace(time->uri(), time);

	InternalPlugin* trigger = internals::TriggerNode::internal_plugin(uris);
	_plugins.emplace(trigger->uri(), trigger);
}

void
BlockFactory::load_plugin(const URI& uri)
{
	if (_has_loaded || _plugins.find(uri) != _plugins.end()) {
		return;
	}

	LilvNode*          node  = lilv_new_uri(_world.lilv_world(), uri.c_str());
	const LilvPlugins* plugs = lilv_world_get_all_plugins(_world.lilv_world());
	const LilvPlugin*  plug  = lilv_plugins_get_by_uri(plugs, node);
	if (plug) {
		auto* const ingen_plugin = new LV2Plugin(_world, plug);
		_plugins.emplace(uri, ingen_plugin);
	}
	lilv_node_free(node);
}

/** Loads information about all LV2 plugins into internal plugin database.
 */
void
BlockFactory::load_lv2_plugins()
{
	// Build an array of port type nodes for checking compatibility
	using Types = std::vector<std::shared_ptr<LilvNode>>;
	Types types;
	for (unsigned t = PortType::ID::AUDIO; t <= PortType::ID::ATOM; ++t) {
		const URI& uri(PortType(static_cast<PortType::ID>(t)).uri());
		types.push_back(std::shared_ptr<LilvNode>(
		    lilv_new_uri(_world.lilv_world(), uri.c_str()), lilv_node_free));
	}

	const LilvPlugins* plugins = lilv_world_get_all_plugins(_world.lilv_world());
	LILV_FOREACH (plugins, i, plugins) {
		const LilvPlugin* lv2_plug = lilv_plugins_get(plugins, i);
		const URI         uri(lilv_node_as_uri(lilv_plugin_get_uri(lv2_plug)));

		// Ignore plugins that require features Ingen doesn't support
		LilvNodes* features  = lilv_plugin_get_required_features(lv2_plug);
		bool       supported = true;
		LILV_FOREACH (nodes, f, features) {
			const char* feature = lilv_node_as_uri(lilv_nodes_get(features, f));
			if (!_world.lv2_features().is_supported(feature)) {
				supported = false;
				_world.log().warn("Ignoring <%1%>; required feature <%2%>\n",
				                  uri, feature);
				break;
			}
		}
		lilv_nodes_free(features);
		if (!supported) {
			continue;
		}

		// Ignore plugins that are missing ports
		if (!lilv_plugin_get_port_by_index(lv2_plug, 0)) {
			_world.log().warn("Ignoring <%1%>; missing or corrupt ports\n",
			                  uri);
			continue;
		}

		const uint32_t n_ports = lilv_plugin_get_num_ports(lv2_plug);
		for (uint32_t p = 0; p < n_ports; ++p) {
			const LilvPort* port = lilv_plugin_get_port_by_index(lv2_plug, p);
			supported = false;
			for (const auto& t : types) {
				if (lilv_port_is_a(lv2_plug, port, t.get())) {
					supported = true;
					break;
				}
			}
			if (!supported &&
			    !lilv_port_has_property(lv2_plug,
			                            port,
			                            _world.uris().lv2_connectionOptional)) {
				_world.log().warn("Ignoring <%1%>; unsupported port <%2%>\n",
				                  uri,
				                  lilv_node_as_string(
					                  lilv_port_get_symbol(lv2_plug, port)));
				break;
			}
		}
		if (!supported) {
			continue;
		}

		auto p = _plugins.find(uri);
		if (p == _plugins.end()) {
			auto* const plugin = new LV2Plugin(_world, lv2_plug);
			_plugins.emplace(uri, plugin);
		} else if (lilv_plugin_verify(lv2_plug)) {
			p->second->set_is_zombie(false);
		}
	}

	_world.log().info("Loaded %1% plugins\n", _plugins.size());
}

} // namespace server
} // namespace ingen
