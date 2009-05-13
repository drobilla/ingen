/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#ifndef HTTPCLIENTSENDER_H
#define HTTPCLIENTSENDER_H

#include <cassert>
#include <string>
#include <iostream>
#include <lo/lo.h>
#include <pthread.h>
#include "raul/Thread.hpp"
#include "interface/ClientInterface.hpp"
#include "shared/HTTPSender.hpp"

namespace Ingen {

class Engine;

namespace Shared { class EngineInterface; }


/** Implements ClientInterface for HTTP clients.
 * Sends changes as RDF deltas over an HTTP stream
 * (a single message with chunked encoding response).
 *
 * \ingroup engine
 */
class HTTPClientSender
	: public Shared::ClientInterface
	, public Shared::HTTPSender
{
public:
	HTTPClientSender(Engine& engine)
		: _engine(engine)
	{}

	bool enabled() const { return _enabled; }

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin()   { HTTPSender::bundle_begin(); }
	void bundle_end()     { HTTPSender::bundle_end(); }
	void transfer_begin() { HTTPSender::transfer_begin(); }
	void transfer_end()   { HTTPSender::transfer_end(); }

	Raul::URI uri() const { return "http://example.org/"; }

    void subscribe(Shared::EngineInterface* engine) { }

	/* *** ClientInterface Implementation Below *** */

	//void client_registration(const std::string& url, int client_id);

	void response_ok(int32_t id);
	void response_error(int32_t id, const std::string& msg);

	void error(const std::string& msg);

	virtual bool new_object(const Shared::GraphObject* object);

	virtual void new_plugin(const Raul::URI&    uri,
	                        const Raul::URI&    type_uri,
	                        const Raul::Symbol& symbol);

	virtual void new_patch(const Raul::Path& path,
	                       uint32_t          poly);

	virtual void new_node(const Raul::Path& path,
	                      const Raul::URI&  plugin_uri);

	virtual void new_port(const Raul::Path& path,
	                      const Raul::URI&  type,
	                      uint32_t          index,
	                      bool              is_output);

	virtual void clear_patch(const Raul::Path& path);

	virtual void destroy(const Raul::Path& path);

	virtual void rename(const Raul::Path& old_path,
	                    const Raul::Path& new_path);

	virtual void connect(const Raul::Path& src_port_path,
	                     const Raul::Path& dst_port_path);

	virtual void disconnect(const Raul::Path& src_port_path,
	                        const Raul::Path& dst_port_path);

	virtual void set_variable(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void set_property(const Raul::URI&  subject_path,
	                          const Raul::URI&  predicate,
	                          const Raul::Atom& value);

	virtual void set_port_value(const Raul::Path& port_path,
	                            const Raul::Atom& value);

	virtual void set_voice_value(const Raul::Path& port_path,
	                             uint32_t          voice,
	                             const Raul::Atom& value);

	virtual void activity(const Raul::Path& path);

private:
	Engine&     _engine;
	std::string _url;
	bool        _enabled;
};


} // namespace Ingen

#endif // HTTPCLIENTSENDER_H

