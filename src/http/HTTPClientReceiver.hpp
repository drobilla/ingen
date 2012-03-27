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


#ifndef INGEN_CLIENT_HTTPCLIENTRECEIVER_HPP
#define INGEN_CLIENT_HTTPCLIENTRECEIVER_HPP

#include <cstdlib>

#include <boost/utility.hpp>
#include <glibmm/thread.h>

#include "ingen/ClientInterface.hpp"
#include "ingen/serialisation/Parser.hpp"
#include "raul/Deletable.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"
#include "sord/sordmm.hpp"

typedef struct _SoupSession SoupSession;
typedef struct _SoupMessage SoupMessage;

namespace Ingen {
namespace Client {

class HTTPClientReceiver : public boost::noncopyable, public Raul::Deletable
{
public:
	HTTPClientReceiver(Shared::World*             world,
	                   const std::string&         url,
	                   SharedPtr<ClientInterface> target);

	~HTTPClientReceiver();

	void send(SoupMessage* msg);
	void close_session();

	std::string uri() const { return _url; }

	void start(bool dump);
	void stop();

	static void message_callback(SoupSession* session, SoupMessage* msg, void* ptr);

private:
	void update(const std::string& str);

	class Listener : public Raul::Thread {
	public:
		Listener(HTTPClientReceiver* receiver, const std::string& uri);
		~Listener();
		void _run();
	private:
		std::string         _uri;
		int                 _sock;
		HTTPClientReceiver* _receiver;
	};

	friend class Listener;

	SharedPtr<Listener>        _listener;
	Glib::Mutex                _mutex;
	SharedPtr<ClientInterface> _target;
	Shared::World*             _world;
	const std::string          _url;
	SoupSession*               _client_session;
	bool                       _quit_flag;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_HTTPCLIENTRECEIVER_HPP
