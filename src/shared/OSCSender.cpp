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

#include <cassert>
#include <unistd.h>
#include <stdarg.h>
#include "raul/log.hpp"
#include "OSCSender.hpp"
#include "ingen-config.h"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


OSCSender::OSCSender()
	: _send_state(Immediate)
	, _transfer(NULL)
	, _address(NULL)
	, _enabled(true)
{
}


void
OSCSender::bundle_begin()
{
	assert(!_transfer);
	lo_timetag t;
	lo_timetag_now(&t);
	_transfer = lo_bundle_new(t);
	_send_state = SendingBundle;
}


void
OSCSender::bundle_end()
{
	transfer_end();
}


void
OSCSender::transfer_begin()
{
	assert(!_transfer);
	lo_timetag t;
	lo_timetag_now(&t);
	_transfer = lo_bundle_new(t);
	_send_state = SendingTransfer;
}


void
OSCSender::transfer_end()
{
	assert(_transfer);
	lo_send_bundle(_address, _transfer);
	lo_bundle_free_messages(_transfer);
	_transfer = NULL;
	_send_state = Immediate;
}


int
OSCSender::send(const char *path, const char *types, ...)
{
	if (!_enabled)
		return 0;

#ifdef LOG_DEBUG
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
	// FIXME: size?  liblo doesn't export this.
	// Don't want to exceed max UDP packet size (good default value?)
	static const size_t MAX_BUNDLE_SIZE = 1024;

	if (!_enabled) {
		lo_message_free(msg);
		return;
	}

	if (_transfer) {
		if (lo_bundle_length(_transfer) + lo_message_length(msg, path) > MAX_BUNDLE_SIZE) {
			if (_send_state == SendingBundle)
				warn << "Maximum bundle size reached, bundle split" << endl;
			lo_send_bundle(_address, _transfer);
			lo_bundle_free_messages(_transfer);
			lo_timetag t;
			lo_timetag_now(&t);
			_transfer = lo_bundle_new(t);
		}
		lo_bundle_add_message(_transfer, path, msg);

	} else {
		lo_send_message(_address, path, msg);
		lo_message_free(msg);
	}
}

} // namespace Shared
} // namespace Ingen
