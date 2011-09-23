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

#ifndef INGEN_INTERFACE_CLIENTINTERFACE_HPP
#define INGEN_INTERFACE_CLIENTINTERFACE_HPP

#include <stdint.h>

#include <string>

#include "raul/URI.hpp"

#include "ingen/CommonInterface.hpp"

namespace Raul { class Path; }

namespace Ingen {

/** The (only) interface the engine uses to communicate with clients.
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class ClientInterface : public CommonInterface
{
public:
	virtual ~ClientInterface() {}

	virtual Raul::URI uri() const = 0;

	virtual void response_ok(int32_t id) = 0;

	virtual void response_error(int32_t id, const std::string& msg) = 0;

	virtual void error(const std::string& msg) = 0;

	virtual void activity(const Raul::Path& path,
	                      const Raul::Atom& value) = 0;
};

} // namespace Ingen

#endif
