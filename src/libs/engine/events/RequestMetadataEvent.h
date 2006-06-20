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

#ifndef REQUESTMETADATAEVENT_H
#define REQUESTMETADATAEVENT_H

#include <string>
#include "QueuedEvent.h"

using std::string;

namespace Om {
	
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
	RequestMetadataEvent(CountedPtr<Responder> responder, const string& path, const string& key);

	void pre_process();
	void post_process();

private:
	string                      m_path;
	string                      m_key;
	string                      m_value; 
	GraphObject*                   m_object;
	CountedPtr<ClientInterface> m_client;
};


} // namespace Om

#endif // REQUESTMETADATAEVENT_H
