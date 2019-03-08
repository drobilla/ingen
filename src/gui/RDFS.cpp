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

#include "ingen/Forge.hpp"
#include "ingen/Log.hpp"
#include "ingen/Resource.hpp"
#include "ingen/World.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "lilv/lilv.h"

#include "RDFS.hpp"

#include <utility>

namespace ingen {
namespace gui {
namespace rdfs {

std::string
label(World& world, const LilvNode* node)
{
	LilvNode* rdfs_label = lilv_new_uri(
		world.lilv_world(), LILV_NS_RDFS "label");
	LilvNodes* labels = lilv_world_find_nodes(
		world.lilv_world(), node, rdfs_label, nullptr);

	const LilvNode* first = lilv_nodes_get_first(labels);
	std::string     label = first ? lilv_node_as_string(first) : "";

	lilv_nodes_free(labels);
	lilv_node_free(rdfs_label);
	return label;
}

std::string
comment(World& world, const LilvNode* node)
{
	LilvNode* rdfs_comment = lilv_new_uri(
		world.lilv_world(), LILV_NS_RDFS "comment");
	LilvNodes* comments = lilv_world_find_nodes(
		world.lilv_world(), node, rdfs_comment, nullptr);

	const LilvNode* first   = lilv_nodes_get_first(comments);
	std::string     comment = first ? lilv_node_as_string(first) : "";

	lilv_nodes_free(comments);
	lilv_node_free(rdfs_comment);
	return comment;
}

static void
closure(World& world, const LilvNode* pred, URISet& types, bool super)
{
	unsigned added = 0;
	do {
		added = 0;
		URISet klasses;
		for (const auto& t : types) {
			LilvNode*  type    = lilv_new_uri(world.lilv_world(), t.c_str());
			LilvNodes* matches = (super)
				? lilv_world_find_nodes(
					world.lilv_world(), type, pred, nullptr)
				: lilv_world_find_nodes(
					world.lilv_world(), nullptr, pred, type);
			LILV_FOREACH(nodes, m, matches) {
				const LilvNode* klass_node = lilv_nodes_get(matches, m);
				if (lilv_node_is_uri(klass_node)) {
					URI klass(lilv_node_as_uri(klass_node));
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
}

void
classes(World& world, URISet& types, bool super)
{
	LilvNode* rdfs_subClassOf = lilv_new_uri(
		world.lilv_world(), LILV_NS_RDFS "subClassOf");

	closure(world, rdfs_subClassOf, types, super);

	lilv_node_free(rdfs_subClassOf);
}

void
datatypes(World& world, URISet& types, bool super)
{
	LilvNode* owl_onDatatype = lilv_new_uri(
		world.lilv_world(), LILV_NS_OWL "onDatatype");

	closure(world, owl_onDatatype, types, super);

	lilv_node_free(owl_onDatatype);
}

URISet
types(World& world, SPtr<const client::ObjectModel> model)
{
	typedef Properties::const_iterator    PropIter;
	typedef std::pair<PropIter, PropIter> PropRange;

	// Start with every rdf:type
	URISet types;
	types.insert(URI(LILV_NS_RDFS "Resource"));
	PropRange range = model->properties().equal_range(world.uris().rdf_type);
	for (auto t = range.first; t != range.second; ++t) {
		if (t->second.type() == world.forge().URI ||
		    t->second.type() == world.forge().URID) {
			const URI type(world.forge().str(t->second, false));
			types.insert(type);
			if (world.uris().ingen_Graph == type) {
				// Add lv2:Plugin as a type for graphs so plugin properties show up
				types.insert(world.uris().lv2_Plugin);
			}
		} else {
			world.log().error("<%1%> has non-URI type\n", model->uri());
		}
	}

	// Add every superclass of every type, recursively
	rdfs::classes(world, types, true);

	return types;
}

URISet
properties(World& world, SPtr<const client::ObjectModel> model)
{
	URISet properties;
	URISet types = rdfs::types(world, model);

	LilvNode* rdf_type = lilv_new_uri(world.lilv_world(),
	                                  LILV_NS_RDF "type");
	LilvNode* rdf_Property = lilv_new_uri(world.lilv_world(),
	                                      LILV_NS_RDF "Property");
	LilvNode* rdfs_domain = lilv_new_uri(world.lilv_world(),
	                                     LILV_NS_RDFS "domain");

	LilvNodes* props = lilv_world_find_nodes(
		world.lilv_world(), nullptr, rdf_type, rdf_Property);
	LILV_FOREACH(nodes, p, props) {
		const LilvNode* prop = lilv_nodes_get(props, p);
		if (lilv_node_is_uri(prop)) {
			LilvNodes* domains = lilv_world_find_nodes(
				world.lilv_world(), prop, rdfs_domain, nullptr);
			unsigned n_matching_domains = 0;
			LILV_FOREACH(nodes, d, domains) {
				const LilvNode* domain_node = lilv_nodes_get(domains, d);
				if (!lilv_node_is_uri(domain_node)) {
					// TODO: Blank node domains (e.g. unions)
					continue;
				}

				const URI domain(lilv_node_as_uri(domain_node));
				if (types.count(domain)) {
					++n_matching_domains;
				}
			}

			if (lilv_nodes_size(domains) == 0 || (
				    n_matching_domains > 0 &&
				    n_matching_domains == lilv_nodes_size(domains))) {
				properties.insert(URI(lilv_node_as_uri(prop)));
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
instances(World& world, const URISet& types)
{
	LilvNode* rdf_type = lilv_new_uri(
		world.lilv_world(), LILV_NS_RDF "type");

	Objects result;
	for (const auto& t : types) {
		LilvNode*  type    = lilv_new_uri(world.lilv_world(), t.c_str());
		LilvNodes* objects = lilv_world_find_nodes(
			world.lilv_world(), nullptr, rdf_type, type);
		LILV_FOREACH(nodes, o, objects) {
			const LilvNode* object = lilv_nodes_get(objects, o);
			if (!lilv_node_is_uri(object)) {
				continue;
			}
			const std::string label = rdfs::label(world, object);
			result.emplace(label, URI(lilv_node_as_string(object)));
		}
		lilv_node_free(type);
	}

	lilv_node_free(rdf_type);
	return result;
}

URISet
range(World& world, const LilvNode* prop, bool recursive)
{
	LilvNode* rdfs_range = lilv_new_uri(
		world.lilv_world(), LILV_NS_RDFS "range");

	LilvNodes* nodes = lilv_world_find_nodes(
		world.lilv_world(), prop, rdfs_range, nullptr);

	URISet ranges;
	LILV_FOREACH(nodes, n, nodes) {
		ranges.insert(URI(lilv_node_as_string(lilv_nodes_get(nodes, n))));
	}

	if (recursive) {
		rdfs::classes(world, ranges, false);
	}

	lilv_nodes_free(nodes);
	lilv_node_free(rdfs_range);
	return ranges;
}

bool
is_a(World& world, const LilvNode* inst, const LilvNode* klass)
{
	LilvNode* rdf_type = lilv_new_uri(world.lilv_world(), LILV_NS_RDF "type");

	const bool is_instance = lilv_world_ask(
		world.lilv_world(), inst, rdf_type, klass);

	lilv_node_free(rdf_type);
	return is_instance;
}

} // namespace rdfs
} // namespace gui
} // namespace ingen
