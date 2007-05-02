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

#ifndef REQUESTMETADATAEVENT_H
#define REQUESTMETADATAEVENT_H

#include <string>
#include "QueuedEvent.h"
#include <raul/Atom.h>
using std::string;

namespace Ingen {
	
class GraphObject;
namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;


/** A request from a client for a piece of metadata.
 *
 * \ingroup engine
 */
class RequestMetadataEvent : public QueuedEvent
{
public:
	RequestMetadataEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, const string& key);

	void pre_process();
	void post_process();

private:
	string                     _path;
	string                     _key;
	Raul::Atom                 _value; 
	GraphObject*               _object;
	SharedPtr<ClientInterface> _client;
};


} // namespace Ingen

#endif // REQUESTMETADATAEVENT_H
