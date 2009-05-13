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

#ifndef OSCCLIENTSENDER_H
#define OSCCLIENTSENDER_H

#include <cassert>
#include <string>
#include <iostream>
#include <lo/lo.h>
#include <pthread.h>
#include "interface/ClientInterface.hpp"
#include "interface/GraphObject.hpp"
#include "shared/OSCSender.hpp"

namespace Ingen {

namespace Shared { class EngineInterface; }


/** Implements ClientInterface for OSC clients (sends OSC messages).
 *
 * \ingroup engine
 */
class OSCClientSender : public Shared::ClientInterface, public Shared::OSCSender
{
public:
	OSCClientSender(const Raul::URI& url)
		: _url(url)
	{
		_address = lo_address_new_from_url(url.c_str());
	}

	virtual ~OSCClientSender()
	{ lo_address_free(_address); }

	bool enabled() const { return _enabled; }

	void enable()  { _enabled = true; }
	void disable() { _enabled = false; }

	void bundle_begin()   { OSCSender::bundle_begin(); }
	void bundle_end()     { OSCSender::bundle_end(); }
	void transfer_begin() { OSCSender::transfer_begin(); }
	void transfer_end()   { OSCSender::transfer_end(); }

	Raul::URI uri() const { return lo_address_get_url(_address); }

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
	Raul::URI _url;
};


} // namespace Ingen

#endif // OSCCLIENTSENDER_H

