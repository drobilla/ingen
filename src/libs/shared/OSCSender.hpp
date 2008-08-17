/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef OSCSENDER_H
#define OSCSENDER_H

#include <inttypes.h>
#include <lo/lo.h>

namespace Ingen {
namespace Shared {

class OSCSender {
public:
	OSCSender();
	~OSCSender();

	lo_address address() const { return _address; }
	
	// Message bundling
	void bundle_begin();
	void bundle_end();
	
	// Transfers (loose bundling)
	void transfer_begin();
	void transfer_end();

protected:
	int  send(const char *path, const char *types, ...);
	void send_message(const char* path, lo_message m);

	enum SendState { Immediate, SendingBundle, SendingTransfer };

	SendState  _send_state;
	lo_bundle  _transfer;
	lo_address _address;
	bool       _enabled;
};


} // namespace Shared
} // namespace Ingen

#endif // OSCSENDER_H

