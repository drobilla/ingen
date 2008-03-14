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

#ifndef SETMETADATAEVENT_H
#define SETMETADATAEVENT_H

#include <string>
#include "QueuedEvent.hpp"
#include <raul/Atom.hpp>

using std::string;

namespace Ingen {

class GraphObjectImpl;


/** An event to set a piece of variable for an GraphObjectImpl.
 *
 * \ingroup engine
 */
class SetMetadataEvent : public QueuedEvent
{
public:
	SetMetadataEvent(Engine&              engine,
	                 SharedPtr<Responder> responder,
	                 SampleCount          timestamp,
	                 const string&        path,
	                 const string&        key,
	                 const Raul::Atom&    value);
	
	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	string       _path;
	string       _key;
	Raul::Atom   _value;
	GraphObjectImpl* _object;
};


} // namespace Ingen

#endif // SETMETADATAEVENT_H
