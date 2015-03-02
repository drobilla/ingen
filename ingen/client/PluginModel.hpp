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

#ifndef INGEN_CLIENT_PLUGINMODEL_HPP
#define INGEN_CLIENT_PLUGINMODEL_HPP

#include <list>
#include <string>
#include <utility>

#include "ingen/Plugin.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Resource.hpp"
#include "ingen/World.hpp"
#include "ingen/client/signal.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "lilv/lilv.h"
#include "raul/Symbol.hpp"
#include "sord/sordmm.hpp"

namespace Ingen {

class URIs;

namespace Client {

class GraphModel;
class BlockModel;
class PluginUI;

/** Model for a plugin available for loading.
 *
 * @ingroup IngenClient
 */
class INGEN_API PluginModel : public Ingen::Plugin
{
public:
	PluginModel(URIs&                              uris,
	            const Raul::URI&                   uri,
	            const Raul::URI&                   type_uri,
	            const Ingen::Resource::Properties& properties);

	Type type() const { return _type; }

	virtual const Atom& get_property(const Raul::URI& key) const;

	Raul::Symbol default_block_symbol() const;
	std::string  human_name() const;
	std::string  port_human_name(uint32_t index) const;

	typedef std::map<float, std::string> ScalePoints;
	ScalePoints port_scale_points(uint32_t i) const;

	static LilvWorld* lilv_world()        { return _lilv_world; }
	const LilvPlugin* lilv_plugin() const { return _lilv_plugin; }

	const LilvPort* lilv_port(uint32_t index) const;

	static void set_lilv_world(LilvWorld* world);

	bool has_ui() const;

	SPtr<PluginUI> ui(Ingen::World*               world,
	                  SPtr<const BlockModel> block) const;

	std::string documentation(bool html) const;
	std::string port_documentation(uint32_t index, bool html) const;

	static void set_rdf_world(Sord::World& world) {
		_rdf_world = &world;
	}

	static Sord::World* rdf_world() { return _rdf_world; }

	// Signals
	INGEN_SIGNAL(changed, void);
	INGEN_SIGNAL(property, void, const Raul::URI&, const Atom&);

protected:
	friend class ClientStore;
	void set(SPtr<PluginModel> p);

private:
	std::string get_documentation(const LilvNode* subject, bool html) const;

	static Sord::World*       _rdf_world;
	static LilvWorld*         _lilv_world;
	static const LilvPlugins* _lilv_plugins;

	Type              _type;
	const LilvPlugin* _lilv_plugin;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PLUGINMODEL_HPP
