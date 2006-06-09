/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Responder.h"
#include <lo/lo.h>
#include "ClientBroadcaster.h"

namespace Om {

// FIXME FIXME FIXME this is all reeeeally baaaaaadddd
// Need an abstract interface for control band


void
Responder::respond_ok()
{
	assert(_key.type() == ClientKey::OSC);
	lo_address source = lo_address_new_from_url(_key.source_url());

	if (source != NULL) {
		if (lo_send(source, "/om/response/ok", "i", _id) < 0) {
			cerr << "Unable to send response " << _id << "! ("
				<< lo_address_errstr(source) << ")" << endl;
		}
	}

	lo_address_free(source);
}


void
Responder::respond_error(const string& msg)
{
	assert(_key.type() == ClientKey::OSC);
	lo_address source = lo_address_new_from_url(_key.source_url());

	if (source != NULL) {
		if (lo_send(source, "/om/response/error", "is",_id, msg.c_str()) < 0) {
			cerr << "Unable to send response " << _id << "! ("
				<< lo_address_errstr(source) << endl;
		}
	}
	lo_address_free(source);
}

} // namespace OM

