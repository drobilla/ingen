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

#ifndef INGEN_CLIENT_GRAPHMODEL_HPP
#define INGEN_CLIENT_GRAPHMODEL_HPP

#include "ingen/client/BlockModel.hpp"
#include "raul/SharedPtr.hpp"

namespace Ingen {
namespace Client {

class ClientStore;
class EdgeModel;

/** Client's model of a graph.
 *
 * @ingroup IngenClient
 */
class GraphModel : public BlockModel
{
public:
	/* WARNING: Copy constructor creates a shallow copy WRT connections */

	GraphType graph_type() const { return Node::GRAPH; }

	SharedPtr<EdgeModel> get_edge(const Ingen::Node* tail,
	                              const Ingen::Node* head);

	bool     enabled()       const;
	bool     polyphonic()    const;
	uint32_t internal_poly() const;

	// Signals
	INGEN_SIGNAL(new_block, void, SharedPtr<BlockModel>);
	INGEN_SIGNAL(removed_block, void, SharedPtr<BlockModel>);
	INGEN_SIGNAL(new_edge, void, SharedPtr<EdgeModel>);
	INGEN_SIGNAL(removed_edge, void, SharedPtr<EdgeModel>);

private:
	friend class ClientStore;

	GraphModel(URIs& uris, const Raul::Path& graph_path)
		: BlockModel(uris, uris.ingen_Graph, graph_path)
	{}

	void clear();
	void add_child(SharedPtr<ObjectModel> c);
	bool remove_child(SharedPtr<ObjectModel> c);

	void add_edge(SharedPtr<EdgeModel> cm);
	void remove_edge(const Ingen::Node* tail,
	                 const Ingen::Node* head);
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_GRAPHMODEL_HPP