/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but HAVEOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config.h"

#include <iostream>
#include "client.hpp"
#ifdef HAVE_LIBLO
#include "OSCEngineSender.hpp"
#endif
#ifdef HAVE_SOUP
#include "HTTPEngineSender.hpp"
#endif

using namespace std;

namespace Ingen {
namespace Client {
	

SharedPtr<Ingen::Shared::EngineInterface>
new_remote_interface(const std::string& url)
{
	const string scheme = url.substr(0, url.find(":"));

#ifdef HAVE_LIBLO
	if (scheme == "osc.udp" || scheme == "osc.tcp") {
		OSCEngineSender* oes = OSCEngineSender::create(url);
		oes->attach(rand(), true);
		return SharedPtr<Shared::EngineInterface>(oes);
	}
#endif

#ifdef HAVE_SOUP
	if (scheme == "http") {
		HTTPEngineSender* hes = new HTTPEngineSender(url);
		hes->attach(rand(), true);
		return SharedPtr<Shared::EngineInterface>(hes);
	}
#endif

	cerr << "WARNING: Unknown URI scheme '" << scheme << "'" << endl;
	return SharedPtr<Shared::EngineInterface>();
}


} // namespace Client
} // namespace Ingen

