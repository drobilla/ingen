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

#include "ingen/Resource.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "lilv/lilv.h"

#include "RDFS.hpp"

namespace Ingen {
namespace GUI {
namespace RDFS {

Glib::ustring
label(World* world, const LilvNode* node)
{
	LilvNode* rdfs_label = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "label");
	LilvNodes* labels = lilv_world_find_nodes(
		world->lilv_world(), node, rdfs_label, NULL);

	const LilvNode* first = lilv_nodes_get_first(labels);
	Glib::ustring   label = first ? lilv_node_as_string(first) : "";

	lilv_nodes_free(labels);
	lilv_node_free(rdfs_label);
	return label;
}

void
classes(World* world, URISet& types, bool super)
{
	LilvNode* rdfs_subClassOf = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDFS "subClassOf");

	unsigned added = 0;
	do {
		added = 0;
		URISet klasses;
		for (URISet::iterator t = types.begin(); t != types.end(); ++t) {
			LilvNode*  type    = lilv_new_uri(world->lilv_world(), t->c_str());
			LilvNodes* matches = (super)
				? lilv_world_find_nodes(
					world->lilv_world(), type, rdfs_subClassOf, NULL)
				: lilv_world_find_nodes(
					world->lilv_world(), NULL, rdfs_subClassOf, type);
			LILV_FOREACH(nodes, m, matches) {
				const LilvNode* klass_node = lilv_nodes_get(matches, m);
				if (lilv_node_is_uri(klass_node)) {
					Raul::URI klass(lilv_node_as_uri(klass_node));
					if (!types.count(klass)) {
						++added;
						klasses.insert(klass);
					}
				}
			}
			lilv_nodes_free(matches);
			lilv_node_free(type);
		}
		types.insert(klasses.begin(), klasses.end());
	} while (added > 0);

	lilv_node_free(rdfs_subClassOf);
}

URISet
types(World* world, SharedPtr<const Client::ObjectModel> model)
{
	typedef Resource::Properties::const_iterator PropIter;
	typedef std::pair<PropIter, PropIter>        PropRange;

	// Start with every rdf:type
	URISet types;
	PropRange range = model->properties().equal_range(world->uris().rdf_type);
	for (PropIter t = range.first; t != range.second; ++t) {
		types.insert(Raul::URI(t->second.get_uri()));
		if (world->uris().ingen_Graph == t->second.get_uri()) {
			// Add lv2:Plugin as a type for graphs so plugin properties show up
			types.insert(world->uris().lv2_Plugin);
		}
	}

	// Add every superclass of every type, recursively
	RDFS::classes(world, types, true);

	return types;
}

URISet
properties(World* world, SharedPtr<const Client::ObjectModel> model)
{
	URISet properties;
	URISet types = RDFS::types(world, model);

	LilvNode* rdf_type = lilv_new_uri(world->lilv_world(),
	                                  LILV_NS_RDF "type");
	LilvNode* rdf_Property = lilv_new_uri(world->lilv_world(),
	                                      LILV_NS_RDF "Property");
	LilvNode* rdfs_domain = lilv_new_uri(world->lilv_world(),
	                                     LILV_NS_RDFS "domain");

	LilvNodes* props = lilv_world_find_nodes(
		world->lilv_world(), NULL, rdf_type, rdf_Property);
	LILV_FOREACH(nodes, p, props) {
		const LilvNode* prop = lilv_nodes_get(props, p);
		if (lilv_node_is_uri(prop)) {
			LilvNodes* domains = lilv_world_find_nodes(
				world->lilv_world(), prop, rdfs_domain, NULL);
			unsigned n_matching_domains = 0;
			LILV_FOREACH(nodes, d, domains) {
				const LilvNode* domain_node = lilv_nodes_get(domains, d);
				const Raul::URI domain(lilv_node_as_uri(domain_node));
				if (types.count(domain)) {
					++n_matching_domains;
				}
			}

			if (lilv_nodes_size(domains) == 0 || (
				    n_matching_domains > 0 &&
				    n_matching_domains == lilv_nodes_size(domains))) {
				properties.insert(Raul::URI(lilv_node_as_uri(prop)));
			}

			lilv_nodes_free(domains);
		}
	}

	lilv_node_free(rdfs_domain);
	lilv_node_free(rdf_Property);
	lilv_node_free(rdf_type);

	return properties;
}

Objects
instances(World* world, const URISet& types)
{
	LilvNode* rdf_type = lilv_new_uri(
		world->lilv_world(), LILV_NS_RDF "type");

	Objects result;
	for (URISet::const_iterator i = types.begin(); i != types.end(); ++i) {
		LilvNode*  type    = lilv_new_uri(world->lilv_world(), i->c_str());
		LilvNodes* objects = lilv_world_find_nodes(
			world->lilv_world(), NULL, rdf_type, type);
		LILV_FOREACH(nodes, o, objects) {
			const LilvNode*     object = lilv_nodes_get(objects, o);
			const Glib::ustring label  = RDFS::label(world, object);
			result.insert(
				std::make_pair(
					Raul::URI(lilv_node_as_string(object)),
					label));
		}
		lilv_node_free(type);
	}

	lilv_node_free(rdf_type);
	return result;
}

} // namespace RDFS
} // namespace GUI
} // namespace Ingen
