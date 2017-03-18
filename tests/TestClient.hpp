/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_TESTCLIENT_HPP
#define INGEN_TESTCLIENT_HPP

#include "ingen/Interface.hpp"

using namespace Ingen;

class TestClient : public Ingen::Interface
{
public:
	explicit TestClient(Log& log) : _log(log) {}
	~TestClient() {}

	Raul::URI uri() const { return Raul::URI("ingen:testClient"); }

	void bundle_begin() {}

	void bundle_end() {}

	void put(const Raul::URI&  uri,
	         const Properties& properties,
	         Resource::Graph   ctx = Resource::Graph::DEFAULT) {}

	void delta(const Raul::URI&  uri,
	           const Properties& remove,
	           const Properties& add) {}

	void copy(const Raul::URI& old_uri,
	          const Raul::URI& new_uri) {}

	void move(const Raul::Path& old_path,
	          const Raul::Path& new_path) {}

	void del(const Raul::URI& uri) {}

	void connect(const Raul::Path& tail,
	             const Raul::Path& head) {}

	void disconnect(const Raul::Path& tail,
	                const Raul::Path& head) {}

	void disconnect_all(const Raul::Path& parent_patch_path,
	                    const Raul::Path& path) {}

	void set_property(const Raul::URI& subject,
	                  const Raul::URI& predicate,
	                  const Atom&      value) {}

	void set_response_id(int32_t id) {}

	void get(const Raul::URI& uri) {}

	void undo() {}

	void redo() {}

	void response(int32_t id, Status status, const std::string& subject) {
		if (status != Status::SUCCESS) {
			_log.error(fmt("error on message %1%: %2% (%3%)\n")
			           % id % ingen_status_string(status) % subject);
			exit(EXIT_FAILURE);
		}
	}

	void error(const std::string& msg) {
		_log.error(fmt("error: %1%\n") % msg);
		exit(EXIT_FAILURE);
	}

private:
	Log& _log;
};

#endif // INGEN_TESTCLIENT_HPP
