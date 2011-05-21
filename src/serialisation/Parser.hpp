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

#ifndef INGEN_SERIALISATION_PARSER_HPP
#define INGEN_SERIALISATION_PARSER_HPP

#include <string>
#include <list>

#include <boost/optional.hpp>
#include <glibmm/ustring.h>

#include "raul/Path.hpp"
#include "serd/serd.h"

#include "ingen/GraphObject.hpp"

namespace Sord  { class World; class Model; class Node; }

namespace Ingen {

class CommonInterface;

namespace Shared { class World; }

namespace Serialisation {

class Parser {
public:
	Parser(Shared::World& world);

	virtual ~Parser() {}

	typedef GraphObject::Properties Properties;

	virtual bool parse_file(
		Shared::World*                world,
		CommonInterface*              target,
		Glib::ustring                 document_uri,
		boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data   = boost::optional<Properties>());

	virtual bool parse_string(
		Shared::World*                world,
		CommonInterface*              target,
		const Glib::ustring&          str,
		const Glib::ustring&          base_uri,
		boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data   = boost::optional<Properties>());

	struct PatchRecord {
		PatchRecord(const Raul::URI& u, const Glib::ustring& f)
			: patch_uri(u), file_uri(f)
		{}
		const Raul::URI     patch_uri;
		const Glib::ustring file_uri;
	};

	typedef std::list<PatchRecord> PatchRecords;

	virtual PatchRecords find_patches(Shared::World*       world,
	                                  SerdEnv*             env,
	                                  const Glib::ustring& manifest_uri);

private:
	boost::optional<Raul::Path> parse(
		Shared::World*                world,
		CommonInterface*              target,
		Sord::Model&                  model,
		Glib::ustring                 document_uri,
		boost::optional<Raul::Path>   data_path = boost::optional<Raul::Path>(),
		boost::optional<Raul::Path>   parent    = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol    = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data      = boost::optional<Properties>());

	boost::optional<Raul::Path> parse_patch(
		Shared::World*                world,
		CommonInterface*              target,
		Sord::Model&                  model,
		const Sord::Node&             subject,
		boost::optional<Raul::Path>   parent = boost::optional<Raul::Path>(),
		boost::optional<Raul::Symbol> symbol = boost::optional<Raul::Symbol>(),
		boost::optional<Properties>   data   = boost::optional<Properties>());

	boost::optional<Raul::Path> parse_node(
		Shared::World*              world,
		CommonInterface*            target,
		Sord::Model&                model,
		const Sord::Node&           subject,
		const Raul::Path&           path,
		boost::optional<Properties> data = boost::optional<Properties>());

	bool parse_properties(
		Shared::World*              world,
		CommonInterface*            target,
		Sord::Model&                model,
		const Sord::Node&           subject,
		const Raul::URI&            uri,
		boost::optional<Properties> data = boost::optional<Properties>());

	bool parse_connections(
		Shared::World*    world,
		CommonInterface*  target,
		Sord::Model&      model,
		const Sord::Node& subject,
		const Raul::Path& patch);
};

} // namespace Serialisation
} // namespace Ingen

#endif // INGEN_SERIALISATION_PARSER_HPP
