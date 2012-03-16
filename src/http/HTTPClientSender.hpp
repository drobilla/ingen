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

#ifndef INGEN_ENGINE_HTTPCLIENTSENDER_HPP
#define INGEN_ENGINE_HTTPCLIENTSENDER_HPP

#include <cassert>
#include <string>

#include "raul/Thread.hpp"

#include "ingen/ClientInterface.hpp"

#include "HTTPSender.hpp"

namespace Ingen {

class ServerInterface;

namespace Server {

class Engine;

/** Implements ClientInterface for HTTP clients.
 * Sends changes as RDF deltas over an HTTP stream
 * (a single message with chunked encoding response).
 *
 * \ingroup engine
 */
class HTTPClientSender
	: public ClientInterface
	, public Ingen::Shared::HTTPSender
{
public:
	explicit HTTPClientSender(Engine& engine)
		: _engine(engine)
		, _enabled(true)
	{}

	bool enabled() const { return _enabled; }

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin() { HTTPSender::bundle_begin(); }
	void bundle_end()   { HTTPSender::bundle_end(); }

	Raul::URI uri() const { return "http://example.org/"; }

	/* *** ClientInterface Implementation Below *** */

	void response(int32_t id, Status status);

	void error(const std::string& msg);

	virtual void put(const Raul::URI&            path,
	                 const Resource::Properties& properties,
	                 Resource::Graph             ctx);

	virtual void delta(const Raul::URI&            path,
	                   const Resource::Properties& remove,
	                   const Resource::Properties& add);

	virtual void del(const Raul::URI& uri);

	virtual void move(const Raul::Path& old_path,
	                  const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::URI& src,
	                        const Raul::URI& dst);

	virtual void disconnect_all(const Raul::Path& parent_patch_path,
	                            const Raul::Path& path);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

private:
	Engine&     _engine;
	std::string _url;
	bool        _enabled;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_HTTPCLIENTSENDER_HPP

