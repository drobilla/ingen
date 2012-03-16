/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_INTERFACE_SERVERINTERFACE_HPP
#define INGEN_INTERFACE_SERVERINTERFACE_HPP

#include <stdint.h>

#include "ingen/CommonInterface.hpp"

namespace Ingen {

class ClientInterface;

/** The (only) interface clients use to communicate with the engine.
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class ServerInterface : public CommonInterface
{
public:
	virtual ~ServerInterface() {}

	/** Set the ID to use to respond to the next message.
	 * Setting the ID to -1 will disable responses.
	 */
	virtual void set_response_id(int32_t id) = 0;

	// Requests
	virtual void ping() = 0;
	virtual void get(const Raul::URI& uri) = 0;
};

} // namespace Ingen

#endif // INGEN_INTERFACE_SERVERINTERFACE_HPP

