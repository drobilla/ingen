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

#include <ingen/Node.hpp>
#include <ingen/client/BlockModel.hpp>
#include <ingen/client/signal.hpp>
#include <ingen/ingen.h>

#include <cstdint>
#include <memory>

namespace raul {
class Path;
} // namespace raul

namespace ingen {

class URIs;

namespace client {

class ArcModel;
class ObjectModel;
class PortModel;

/** Client's model of a graph.
 *
 * @ingroup IngenClient
 */
class INGEN_API GraphModel : public BlockModel
{
public:
	/* WARNING: Copy constructor creates a shallow copy WRT connections */

	GraphType graph_type() const override { return Node::GraphType::GRAPH; }

	std::shared_ptr<ArcModel>
	get_arc(const ingen::Node* tail, const ingen::Node* head);

	bool     enabled()       const;
	uint32_t internal_poly() const;

	// Signals
	INGEN_SIGNAL(new_block, void, std::shared_ptr<BlockModel>)
	INGEN_SIGNAL(removed_block, void, std::shared_ptr<BlockModel>)
	INGEN_SIGNAL(new_arc, void, std::shared_ptr<ArcModel>)
	INGEN_SIGNAL(removed_arc, void, std::shared_ptr<ArcModel>)

private:
	friend class ClientStore;

	GraphModel(URIs& uris, const raul::Path& graph_path);

	void clear() override;
	void add_child(const std::shared_ptr<ObjectModel>& c) override;
	bool remove_child(const std::shared_ptr<ObjectModel>& o) override;
	void remove_arcs_on(const std::shared_ptr<PortModel>& p);

	void add_arc(const std::shared_ptr<ArcModel>& arc);
	void remove_arc(const ingen::Node* tail,
	                const ingen::Node* head);
};

} // namespace client
} // namespace ingen

#endif // INGEN_CLIENT_GRAPHMODEL_HPP
