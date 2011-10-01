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

#ifndef INGEN_CLIENT_PLUGINMODEL_HPP
#define INGEN_CLIENT_PLUGINMODEL_HPP

#include "lilv/lilv.h"
#include "raul/SharedPtr.hpp"
#include "raul/Symbol.hpp"
#include "sord/sordmm.hpp"

#include "ingen/Plugin.hpp"
#include "ingen/ServerInterface.hpp"
#include "ingen/client/signal.hpp"
#include "shared/ResourceImpl.hpp"
#include "shared/World.hpp"

namespace Ingen {

namespace Shared { class LV2URIMap; }

namespace Client {

class PatchModel;
class NodeModel;
class PluginUI;

/** Model for a plugin available for loading.
 *
 * \ingroup IngenClient
 */
class PluginModel : public Ingen::Plugin
                  , public Ingen::Shared::ResourceImpl
{
public:
	PluginModel(
			Shared::LV2URIMap&                 uris,
			const Raul::URI&                   uri,
			const Raul::URI&                   type_uri,
			const Ingen::Resource::Properties& properties);

	Type type() const { return _type; }

	virtual const Raul::Atom& get_property(const Raul::URI& key) const;

	Raul::Symbol default_node_symbol() const;
	std::string  human_name() const;
	std::string  port_human_name(uint32_t index) const;

	static LilvWorld* lilv_world()        { return _lilv_world; }
	const LilvPlugin* lilv_plugin() const { return _lilv_plugin; }

	const LilvPort* lilv_port(uint32_t index) const;

	static void set_lilv_world(LilvWorld* world);

	bool has_ui() const;

	SharedPtr<PluginUI> ui(Ingen::Shared::World*       world,
	                       SharedPtr<const NodeModel>  node) const;

	const std::string& icon_path() const;
	static std::string get_lv2_icon_path(const LilvPlugin* plugin);

	std::string documentation(bool* html) const;
	std::string port_documentation(uint32_t index) const;

	static void set_rdf_world(Sord::World& world) {
		_rdf_world = &world;
	}

	static Sord::World* rdf_world() { return _rdf_world; }

	// Signals
	INGEN_SIGNAL(changed, void);
	INGEN_SIGNAL(property, void, const Raul::URI&, const Raul::Atom&);

protected:
	friend class ClientStore;
	void set(SharedPtr<PluginModel> p);

private:
	Type _type;

	static LilvWorld*         _lilv_world;
	static const LilvPlugins* _lilv_plugins;

	const LilvPlugin*   _lilv_plugin;
	mutable std::string _icon_path;

	static Sord::World* _rdf_world;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PLUGINMODEL_HPP

