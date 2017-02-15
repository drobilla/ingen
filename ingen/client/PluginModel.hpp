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

#ifndef INGEN_CLIENT_PLUGINMODEL_HPP
#define INGEN_CLIENT_PLUGINMODEL_HPP

#include <list>
#include <map>
#include <string>
#include <utility>

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
class INGEN_API PluginModel : public Ingen::Resource
{
public:
	PluginModel(URIs&                    uris,
	            const Raul::URI&         uri,
	            const Atom&              type,
	            const Ingen::Properties& properties);

	const Atom&     type()     const { return _type; }
	const Raul::URI type_uri() const {
		return Raul::URI(_type.is_valid()
		                 ? _uris.forge.str(_type)
		                 : "http://www.w3.org/2002/07/owl#Nothing");
	}

	virtual const Atom& get_property(const Raul::URI& key) const;

	Raul::Symbol default_block_symbol() const;
	std::string  human_name() const;
	std::string  port_human_name(uint32_t index) const;

	typedef std::map<float, std::string> ScalePoints;
	ScalePoints port_scale_points(uint32_t i) const;

	typedef std::map<Raul::URI, std::string> Presets;
	const Presets& presets() const { return _presets; }

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
	INGEN_SIGNAL(preset, void, const Raul::URI&, const std::string&);

	bool fetched() const     { return _fetched; }
	void set_fetched(bool f) { _fetched = f; }

protected:
	friend class ClientStore;
	void set(SPtr<PluginModel> p);

	void add_preset(const Raul::URI& uri, const std::string& label);

private:
	std::string get_documentation(const LilvNode* subject, bool html) const;

	static Sord::World*       _rdf_world;
	static LilvWorld*         _lilv_world;
	static const LilvPlugins* _lilv_plugins;

	Atom              _type;
	const LilvPlugin* _lilv_plugin;
	Presets           _presets;
	bool              _fetched;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PLUGINMODEL_HPP
