/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef HTTPCLIENTRECEIVER_H
#define HTTPCLIENTRECEIVER_H

#include <cstdlib>
#include <boost/utility.hpp>
#include <libsoup/soup.h>
#include "redlandmm/World.hpp"
#include "raul/Deletable.hpp"
#include "raul/Thread.hpp"
#include "interface/ClientInterface.hpp"
#include "serialisation/Parser.hpp"

namespace Ingen {
namespace Client {


class HTTPClientReceiver : public boost::noncopyable, public Raul::Deletable
{
public:
	HTTPClientReceiver(Shared::World*                     world,
	                   const std::string&                 url,
	                   SharedPtr<Shared::ClientInterface> target);

	~HTTPClientReceiver();

	std::string uri() const { return _url; }

	void start(bool dump);
	void stop();
	
private:
	static void message_callback(SoupSession* session, SoupMessage* msg, void* ptr);

	class Listener : public Raul::Thread {
	public:
		Listener(SoupSession* session, SoupMessage* msg) : _session(session), _msg(msg) {}
		void _run();
	private:
		SoupSession* _session;
		SoupMessage* _msg;
	};
	
	friend class Listener;
	SharedPtr<Listener> _listener;

	SharedPtr<Shared::ClientInterface> _target;

	Shared::World*    _world;
	const std::string _url;
	SoupSession*      _session;
	SharedPtr<Parser> _parser;
};


} // namespace Client
} // namespace Ingen

#endif // HTTPCLIENTRECEIVER_H
