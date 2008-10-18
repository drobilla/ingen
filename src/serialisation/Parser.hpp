/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include "raul/SharedPtr.hpp"
#include "raul/Path.hpp"
#include "raul/Table.hpp"
#include "interface/GraphObject.hpp"
#include "module/World.hpp"

namespace Redland { class World; class Model; }
namespace Ingen { namespace Shared { class CommonInterface; } }

using namespace Ingen::Shared;

namespace Ingen {
namespace Serialisation {


class Parser {
public:
	virtual ~Parser() {}
	
	virtual bool parse_document(
			Ingen::Shared::World*                   world,
			Shared::CommonInterface*                target,
			const Glib::ustring&                    document_uri,
			Glib::ustring                           engine_base,
			Glib::ustring                           object_uri,
			boost::optional<Raul::Symbol>           symbol=boost::optional<Raul::Symbol>(),
			boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>());
	
	virtual bool parse_string(
			Ingen::Shared::World*                   world,
			Shared::CommonInterface*                target,
			const Glib::ustring&                    str,
			const Glib::ustring&                    base_uri,
			Glib::ustring                           engine_base,
			boost::optional<Glib::ustring>          object_uri=boost::optional<Glib::ustring>(),
			boost::optional<Raul::Symbol>           symbol=boost::optional<Raul::Symbol>(),
			boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>());

private:

	Glib::ustring uri_relative_to_base(Glib::ustring base, const Glib::ustring uri);

	bool parse(
			Ingen::Shared::World*                   world,
			Shared::CommonInterface*                target,
			Redland::Model&                         model,
			Glib::ustring                           base_uri,
			Glib::ustring                           engine_base,
			boost::optional<Glib::ustring>          object_uri=boost::optional<Glib::ustring>(),
			boost::optional<Raul::Symbol>           symbol=boost::optional<Raul::Symbol>(),
			boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>());

	bool parse_patch(
			Ingen::Shared::World*                   world,
			Ingen::Shared::CommonInterface*         target,
			Redland::Model&                         model,
			const Glib::ustring&                    base_uri,
			Glib::ustring                           engine_base,
			const Glib::ustring&                    object_uri,
			boost::optional<GraphObject::Variables> data);
	
	bool parse_node(
			Ingen::Shared::World*                   world,
			Ingen::Shared::CommonInterface*         target,
			Redland::Model&                         model,
			const Glib::ustring&                    base_uri,
			const Glib::ustring&                    subject,
			const Raul::Path&                       path,
			boost::optional<GraphObject::Variables> data);

	bool parse_port(
			Ingen::Shared::World*                   world,
			Ingen::Shared::CommonInterface*         target,
			Redland::Model&                         model,
			const Glib::ustring&                    base_uri,
			const Glib::ustring&                    subject,
			const Raul::Path&                       path,
			boost::optional<GraphObject::Variables> data=boost::optional<GraphObject::Variables>());
	
	bool parse_variables(
			Ingen::Shared::World*                   world,
			Ingen::Shared::CommonInterface*         target,
			Redland::Model&                         model,
			const Glib::ustring&                    base_uri,
			const Glib::ustring&                    subject,
			const Raul::Path&                       path,
			boost::optional<GraphObject::Variables> data);

	bool parse_connections(
			Ingen::Shared::World*                   world,
			Ingen::Shared::CommonInterface*         target,
			Redland::Model&                         model,
			const Glib::ustring&                    base_uri,
			const Glib::ustring&                    subject,
			const Raul::Path&                       parent);

};


} // namespace Serialisation
} // namespace Ingen

#endif // LOADER_H
