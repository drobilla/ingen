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

#include <ingen/Interface.hpp>
#include <ingen/Log.hpp>
#include <ingen/Message.hpp>
#include <ingen/Status.hpp>
#include <ingen/URI.hpp>

#include <variant>

#include <cstdlib>

namespace ingen {

class TestClient : public Interface
{
public:
	explicit TestClient(Log& log) noexcept : _log(log) {}

	~TestClient() override = default;

	URI uri() const override { return URI("ingen:testClient"); }

	void message(const Message& msg) override {
		if (const Response* const response = std::get_if<Response>(&msg)) {
			if (response->status != Status::SUCCESS) {
				_log.error("error on message %1%: %2% (%3%)\n",
				           response->id,
				           ingen_status_string(response->status),
				           response->subject);
				exit(EXIT_FAILURE);
			}
		} else if (const Error* const error = std::get_if<Error>(&msg)) {
			_log.error("error: %1%\n", error->message);
			exit(EXIT_FAILURE);
		}
	}

private:
	Log& _log;
};

} // namespace ingen

#endif // INGEN_TESTCLIENT_HPP
