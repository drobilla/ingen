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

#ifndef CLIENTINTERFACE_H
#define CLIENTINTERFACE_H

#include <stdio.h>
#include <string>
#include <inttypes.h>
#include "interface/CommonInterface.hpp"

namespace Raul { class Path; class URI; }

namespace Ingen {
namespace Shared {


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

	/** Transfers are 'weak' bundles.  These are used to break a large group
	 * of similar/related messages into larger chunks (solely for communication
	 * efficiency).  A bunch of messages in a transfer will arrive as 1 or more
	 * bundles (so a transfer can exceed the maximum bundle (packet) size).
	 */
	virtual void transfer_begin() = 0;
	virtual void transfer_end()   = 0;

	virtual void error(const std::string& msg) = 0;

	virtual void new_plugin(const Raul::URI&    uri,
	                        const Raul::URI&    type_uri,
	                        const Raul::Symbol& symbol) = 0;

	virtual void activity(const Raul::Path& path) = 0;
};


} // namespace Shared
} // namespace Ingen

#endif
