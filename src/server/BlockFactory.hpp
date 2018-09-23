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

#ifndef INGEN_ENGINE_BLOCKFACTORY_HPP
#define INGEN_ENGINE_BLOCKFACTORY_HPP

#include <map>
#include <set>

#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "raul/Noncopyable.hpp"

namespace ingen {
namespace server {

class PluginImpl;

/** Discovers and loads plugin libraries.
 *
 * \ingroup engine
 */
class BlockFactory : public Raul::Noncopyable
{
public:
	explicit BlockFactory(ingen::World* world);
	~BlockFactory();

	/** Reload plugin list.
	 *
	 * @return The set of newly loaded plugins.
	 */
	std::set<PluginImpl*> refresh();

	void load_plugin(const URI& uri);

	typedef std::map<URI, PluginImpl*> Plugins;
	const Plugins& plugins();

	PluginImpl* plugin(const URI& uri);

private:
	void load_lv2_plugins();
	void load_internal_plugins();

	Plugins       _plugins;
	ingen::World* _world;
	bool          _has_loaded;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_BLOCKFACTORY_HPP
