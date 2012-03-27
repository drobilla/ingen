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

#ifndef INGEN_SHARED_HTTPSENDER_HPP
#define INGEN_SHARED_HTTPSENDER_HPP

#include <string>

#include <glibmm/thread.h>

#include "raul/Thread.hpp"

namespace Ingen {
namespace Shared {

class HTTPSender : public Raul::Thread {
public:
	HTTPSender();
	virtual ~HTTPSender();

	void bundle_begin();
	void bundle_end();

	int listen_port() const { return _listen_port; }

protected:
	void _run();

	void send_chunk(const std::string& buf);

	enum SendState { Immediate, SendingBundle };

	Glib::Mutex _mutex;
	Glib::Cond  _signal;

	int          _listen_port;
	int          _listen_sock;
	int          _client_sock;
	SendState    _send_state;
	std::string  _transfer;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_HTTPSENDER_HPP

