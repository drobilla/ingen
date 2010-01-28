/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef LOADER_H
#define LOADER_H

#include <string>
#include <glibmm/ustring.h>
#include <boost/optional.hpp>
#include "interface/GraphObject.hpp"
#include "module/World.hpp"

namespace Raul { class Path; }
namespace Redland { class World; class Model; class Node; }
namespace Ingen { namespace Shared { class CommonInterface; } }

namespace Ingen {
namespace Serialisation {


class Parser {
public:
	virtual ~Parser() {}

	typedef Shared::GraphObject::Properties Properties;

	virtual bool parse_document(
			Ingen::Shared::World*         world,
			Shared::CommonInterface*      target,
			Glib::ustring                 document_uri,
			boost::optional<Raul::Path>   data_path=boost::optional<Raul::Path>(),
			boost::optional<Raul::Path>   parent=boost::optional<Raul::Path>(),
			boost::optional<Raul::Symbol> symbol=boost::optional<Raul::Symbol>(),
			boost::optional<Properties>   data=boost::optional<Properties>());

	virtual bool parse_string(
			Ingen::Shared::World*         world,
			Shared::CommonInterface*      target,
			const Glib::ustring&          str,
			const Glib::ustring&          base_uri,
			boost::optional<Raul::Path>   data_path=boost::optional<Raul::Path>(),
			boost::optional<Raul::Path>   parent=boost::optional<Raul::Path>(),
			boost::optional<Raul::Symbol> symbol=boost::optional<Raul::Symbol>(),
			boost::optional<Properties>   data=boost::optional<Properties>());

	virtual bool parse_update(
			Ingen::Shared::World*         world,
			Shared::CommonInterface*      target,
			const Glib::ustring&          str,
			const Glib::ustring&          base_uri,
			boost::optional<Raul::Path>   data_path=boost::optional<Raul::Path>(),
			boost::optional<Raul::Path>   parent=boost::optional<Raul::Path>(),
			boost::optional<Raul::Symbol> symbol=boost::optional<Raul::Symbol>(),
			boost::optional<Properties>   data=boost::optional<Properties>());

private:
	boost::optional<Raul::Path> parse(
			Ingen::Shared::World*         world,
			Shared::CommonInterface*      target,
			Redland::Model&               model,
			Glib::ustring                 document_uri,
			boost::optional<Raul::Path>   data_path=boost::optional<Raul::Path>(),
			boost::optional<Raul::Path>   parent=boost::optional<Raul::Path>(),
			boost::optional<Raul::Symbol> symbol=boost::optional<Raul::Symbol>(),
			boost::optional<Properties>   data=boost::optional<Properties>());

	boost::optional<Raul::Path> parse_patch(
			Ingen::Shared::World*           world,
			Ingen::Shared::CommonInterface* target,
			Redland::Model&                 model,
			const Redland::Node&            subject,
			boost::optional<Raul::Path>     parent=boost::optional<Raul::Path>(),
			boost::optional<Raul::Symbol>   symbol=boost::optional<Raul::Symbol>(),
			boost::optional<Properties>     data=boost::optional<Properties>());

	boost::optional<Raul::Path> parse_node(
			Ingen::Shared::World*           world,
			Ingen::Shared::CommonInterface* target,
			Redland::Model&                 model,
			const Redland::Node&            subject,
			const Raul::Path&               path,
			boost::optional<Properties>     data=boost::optional<Properties>());

	bool parse_properties(
			Ingen::Shared::World*           world,
			Ingen::Shared::CommonInterface* target,
			Redland::Model&                 model,
			const Redland::Node&            subject,
			const Raul::URI&                uri,
			boost::optional<Properties>     data=boost::optional<Properties>());

	bool parse_connections(
			Ingen::Shared::World*           world,
			Ingen::Shared::CommonInterface* target,
			Redland::Model&                 model,
			const Redland::Node&            subject,
			const Raul::Path&               patch);

	bool skip_property(const Redland::Node& predicate);
};


} // namespace Serialisation
} // namespace Ingen

#endif // LOADER_H
