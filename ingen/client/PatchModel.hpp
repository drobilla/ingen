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

#ifndef INGEN_CLIENT_PATCHMODEL_HPP
#define INGEN_CLIENT_PATCHMODEL_HPP

#include "ingen/client/NodeModel.hpp"
#include "raul/SharedPtr.hpp"

namespace Ingen {
namespace Client {

class ClientStore;
class EdgeModel;

/** Client's model of a patch.
 *
 * @ingroup IngenClient
 */
class PatchModel : public NodeModel
{
public:
	/* WARNING: Copy constructor creates a shallow copy WRT connections */

	GraphType graph_type() const { return GraphObject::PATCH; }

	SharedPtr<EdgeModel> get_edge(const Ingen::GraphObject* tail,
	                              const Ingen::GraphObject* head);

	bool     enabled()       const;
	bool     polyphonic()    const;
	uint32_t internal_poly() const;

	// Signals
	INGEN_SIGNAL(new_node, void, SharedPtr<NodeModel>);
	INGEN_SIGNAL(removed_node, void, SharedPtr<NodeModel>);
	INGEN_SIGNAL(new_edge, void, SharedPtr<EdgeModel>);
	INGEN_SIGNAL(removed_edge, void, SharedPtr<EdgeModel>);

private:
	friend class ClientStore;

	PatchModel(URIs& uris, const Raul::Path& patch_path)
		: NodeModel(
			uris, Raul::URI("http://drobilla.net/ns/ingen#Patch"), patch_path)
	{
	}

	void clear();
	void add_child(SharedPtr<ObjectModel> c);
	bool remove_child(SharedPtr<ObjectModel> c);

	void add_edge(SharedPtr<EdgeModel> cm);
	void remove_edge(const Ingen::GraphObject* tail,
	                 const Ingen::GraphObject* head);
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PATCHMODEL_HPP
