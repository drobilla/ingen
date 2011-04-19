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

#include <assert.h>
#include <unistd.h>
#include <stdarg.h>

#include "raul/log.hpp"

#include "OSCSender.hpp"
#include "ingen-config.h"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

OSCSender::OSCSender(size_t max_packet_size)
	: _bundle(NULL)
	, _address(NULL)
	, _max_packet_size(max_packet_size)
	, _enabled(true)
{
}

void
OSCSender::bundle_begin()
{
	assert(!_bundle);
	lo_timetag t;
	lo_timetag_now(&t);
	_bundle = lo_bundle_new(t);
}

void
OSCSender::bundle_end()
{
	assert(_bundle);
	lo_send_bundle(_address, _bundle);
	lo_bundle_free_messages(_bundle);
	_bundle = NULL;
}

int
OSCSender::send(const char *path, const char *types, ...)
{
	if (!_enabled)
		return 0;

#ifdef RAUL_LOG_DEBUG
	info << "[OSCSender] " << path << " (" << types << ")" << endl;
#endif

	va_list args;
	va_start(args, types);

	lo_message msg = lo_message_new();
	int ret = lo_message_add_varargs(msg, types, args);

	if (!ret)
		send_message(path, msg);
	else
		lo_message_free(msg);

	va_end(args);

	return ret;
}

void
OSCSender::send_message(const char* path, lo_message msg)
{
	if (!_enabled) {
		lo_message_free(msg);
		return;
	}

	if (_bundle) {
		if (lo_bundle_length(_bundle) + lo_message_length(msg, path) > _max_packet_size) {
			warn << "Maximum bundle size reached, bundle split" << endl;
			lo_send_bundle(_address, _bundle);
			lo_bundle_free_messages(_bundle);
			lo_timetag t;
			lo_timetag_now(&t);
			_bundle = lo_bundle_new(t);
		}
		lo_bundle_add_message(_bundle, path, msg);

	} else {
		lo_send_message(_address, path, msg);
		lo_message_free(msg);
	}
}

} // namespace Shared
} // namespace Ingen
