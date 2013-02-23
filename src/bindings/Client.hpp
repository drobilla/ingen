/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>

#include "ingen/Resource.hpp"

/** Need a stub ClientInterface without pure virtual methods
 * to allow inheritance in the script
 */
class Client : public Ingen::ClientInterface
{
public:
    /** Wrapper for engine->register_client to appease SWIG */
    virtual void subscribe(Ingen::ServerInterface* engine) {
        engine->register_client(this);
    }

	void bundle_begin() {}
	void bundle_end() {}
	void response(int32_t id, Status status, const std::string& subject) {}
	void error(const std::string& msg) {}
	void put(const Raul::URI& path, const Ingen::Resource::Properties& properties) {}
	void connect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path) {}
	void del(const Raul::URI& uri) {}
	void move(const Raul::Path& old_path, const Raul::Path& new_path) {}
	void disconnect(const Raul::Path& src_port_path, const Raul::Path& dst_port_path) {}
	void set_property(const Raul::URI& subject, const Raul::URI& key, const Atom& value) {}
};
