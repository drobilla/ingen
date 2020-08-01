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

#ifndef INGEN_CLIENT_BLOCKMODEL_HPP
#define INGEN_CLIENT_BLOCKMODEL_HPP

#include "ingen/Node.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/client/PluginModel.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Raul { class Path; }

namespace ingen {

class URIs;

namespace client {

class PortModel;

/** Block model class, used by the client to store engine's state.
 *
 * @ingroup IngenClient
 */
class INGEN_API BlockModel : public ObjectModel
{
public:
	BlockModel(const BlockModel& copy);
	virtual ~BlockModel();

	GraphType graph_type() const override { return Node::GraphType::BLOCK; }

	using Ports = std::vector<SPtr<const PortModel>>;

	SPtr<const PortModel> get_port(const Raul::Symbol& symbol) const;
	SPtr<const PortModel> get_port(uint32_t index) const;

	Node* port(uint32_t index) const override;

	const URI&        plugin_uri()   const          { return _plugin_uri; }
	const Resource*   plugin()       const override { return _plugin.get(); }
	Resource*         plugin()                      { return _plugin.get(); }
	SPtr<PluginModel> plugin_model() const          { return _plugin; }
	uint32_t          num_ports()    const override { return _ports.size(); }
	const Ports&      ports()        const          { return _ports; }

	void default_port_value_range(const SPtr<const PortModel>& port,
	                              float&                       min,
	                              float&                       max,
	                              uint32_t                     srate = 1) const;

	void port_value_range(const SPtr<const PortModel>& port,
	                      float&                       min,
	                      float&                       max,
	                      uint32_t                     srate = 1) const;

	std::string label() const;
	std::string port_label(const SPtr<const PortModel>& port) const;

	// Signals
	INGEN_SIGNAL(new_port, void, SPtr<const PortModel>)
	INGEN_SIGNAL(removed_port, void, SPtr<const PortModel>)

protected:
	friend class ClientStore;

	BlockModel(URIs& uris, URI plugin_uri, const Raul::Path& path);

	BlockModel(URIs&                    uris,
	           const SPtr<PluginModel>& plugin,
	           const Raul::Path&        path);

	explicit BlockModel(const Raul::Path& path);

	void add_child(const SPtr<ObjectModel>& c) override;
	bool remove_child(const SPtr<ObjectModel>& c) override;
	void add_port(const SPtr<PortModel>& pm);
	void remove_port(const SPtr<PortModel>& port);
	void remove_port(const Raul::Path& port_path);
	void set(const SPtr<ObjectModel>& model) override;

	virtual void clear();

	Ports             _ports;      ///< Vector of ports
	URI               _plugin_uri; ///< Plugin URI (if PluginModel is unknown)
	SPtr<PluginModel> _plugin;     ///< The plugin this block is an instance of

private:
	mutable uint32_t _num_values; ///< Size of _min_values and _max_values
	mutable float*   _min_values; ///< Port min values (cached for LV2)
	mutable float*   _max_values; ///< Port max values (cached for LV2)
};

} // namespace client
} // namespace ingen

#endif // INGEN_CLIENT_BLOCKMODEL_HPP
