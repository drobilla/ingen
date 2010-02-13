/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software = 0; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation = 0; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY = 0; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program = 0; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_INTERFACE_ENGINEINTERFACE_HPP
#define INGEN_INTERFACE_ENGINEINTERFACE_HPP

#include <inttypes.h>
#include "interface/CommonInterface.hpp"

namespace Ingen {
namespace Shared {

class ClientInterface;


/** The (only) interface clients use to communicate with the engine.
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class EngineInterface : public CommonInterface
{
public:
	virtual ~EngineInterface() {}

	virtual Raul::URI uri() const = 0;

	// Responses
	virtual void set_next_response_id(int32_t id) = 0;
	virtual void disable_responses() = 0;

	// Client registration
	virtual void register_client(ClientInterface* client) = 0;
	virtual void unregister_client(const Raul::URI& uri) = 0;

	// Engine commands
	virtual void load_plugins() = 0;
	virtual void activate() = 0;
	virtual void deactivate() = 0;
	virtual void quit() = 0;

	// Object commands

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path) = 0;

	// Requests

	virtual void ping() = 0;

	virtual void get(const Raul::URI& uri) = 0;

	virtual void request_property(const Raul::URI& uri,
	                              const Raul::URI& key) = 0;

	virtual void request_plugins() = 0;

	virtual void request_all_objects() = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_INTERFACE_ENGINEINTERFACE_HPP

