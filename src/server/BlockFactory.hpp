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

#ifndef INGEN_ENGINE_BLOCKFACTORY_HPP
#define INGEN_ENGINE_BLOCKFACTORY_HPP

#include <map>

#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "raul/Noncopyable.hpp"
#include "raul/URI.hpp"

namespace Ingen {
namespace Server {

class PluginImpl;
class LV2Info;

/** Discovers and loads plugin libraries.
 *
 * \ingroup engine
 */
class BlockFactory : public Raul::Noncopyable
{
public:
	explicit BlockFactory(Ingen::World* world);
	~BlockFactory();

	void load_plugin(const Raul::URI& uri);

	typedef std::map<Raul::URI, PluginImpl*> Plugins;
	const Plugins& plugins();

	PluginImpl* plugin(const Raul::URI& uri);

private:
	void load_lv2_plugins();
	void load_internal_plugins();

	Plugins       _plugins;
	Ingen::World* _world;
	SPtr<LV2Info> _lv2_info;
	bool          _has_loaded;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BLOCKFACTORY_HPP
