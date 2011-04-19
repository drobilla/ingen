/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_INTERFACE_COMMONINTERFACE_HPP
#define INGEN_INTERFACE_COMMONINTERFACE_HPP

#include "ingen/CommonInterface.hpp"
#include "ingen/Resource.hpp"

namespace Raul { class Atom; class Path; class URI; }

namespace Ingen {

/** Abstract interface common to both engine and clients.
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class CommonInterface
{
public:
	virtual ~CommonInterface() {}

	/** Begin an atomic bundle */
	virtual void bundle_begin() = 0;

	/** End (and send) an atomic bundle */
	virtual void bundle_end() = 0;

	virtual void put(const Raul::URI&            uri,
	                 const Resource::Properties& properties,
	                 Resource::Graph             ctx=Resource::DEFAULT) = 0;

	virtual void delta(const Raul::URI&            uri,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add) = 0;

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path) = 0;

	virtual void del(const Raul::URI& uri) = 0;

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path) = 0;

	virtual void disconnect(const Raul::Path& src_port_path,
	                        const Raul::Path& dst_port_path) = 0;

	virtual void set_property(const Raul::URI&  subject,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value) = 0;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_COMMONINTERFACE_HPP

