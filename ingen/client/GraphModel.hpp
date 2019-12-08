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

#ifndef INGEN_CLIENT_GRAPHMODEL_HPP
#define INGEN_CLIENT_GRAPHMODEL_HPP

#include "ingen/Node.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/BlockModel.hpp"
#include "ingen/client/signal.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"

#include <cstdint>

namespace ingen {
namespace client {

class ArcModel;
class ClientStore;

/** Client's model of a graph.
 *
 * @ingroup IngenClient
 */
class INGEN_API GraphModel : public BlockModel
{
public:
	/* WARNING: Copy constructor creates a shallow copy WRT connections */

	GraphType graph_type() const override { return Node::GraphType::GRAPH; }

	SPtr<ArcModel> get_arc(const ingen::Node* tail,
	                       const ingen::Node* head);

	bool     enabled()       const;
	bool     polyphonic()    const;
	uint32_t internal_poly() const;

	// Signals
	INGEN_SIGNAL(new_block, void, SPtr<BlockModel>);
	INGEN_SIGNAL(removed_block, void, SPtr<BlockModel>);
	INGEN_SIGNAL(new_arc, void, SPtr<ArcModel>);
	INGEN_SIGNAL(removed_arc, void, SPtr<ArcModel>);

private:
	friend class ClientStore;

	GraphModel(URIs& uris, const Raul::Path& graph_path)
		: BlockModel(uris, uris.ingen_Graph, graph_path)
	{}

	void clear() override;
	void add_child(const SPtr<ObjectModel>& c) override;
	bool remove_child(const SPtr<ObjectModel>& o) override;
	void remove_arcs_on(const SPtr<PortModel>& p);

	void add_arc(const SPtr<ArcModel>& arc);
	void remove_arc(const ingen::Node* tail,
	                const ingen::Node* head);
};

} // namespace client
} // namespace ingen

#endif // INGEN_CLIENT_GRAPHMODEL_HPP
