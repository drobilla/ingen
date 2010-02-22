/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_REQUESTMETADATA_HPP
#define INGEN_EVENTS_REQUESTMETADATA_HPP

#include "QueuedEvent.hpp"
#include "raul/Atom.hpp"
#include "raul/URI.hpp"

namespace Ingen {

namespace Shared { class ResourceImpl; }
class GraphObjectImpl;

namespace Events {


/** \page methods
 * <h2>GET</h2>
 * As per HTTP (RFC2616 S9.3).
 *
 * Get the description of a graph object.
 */

/** GET an object (see \ref methods).
 *
 * \ingroup engine
 */
class RequestMetadata : public QueuedEvent
{
public:
	RequestMetadata(Engine&            engine,
	                SharedPtr<Request> request,
	                SampleCount        timestamp,
	                bool               meta,
	                const Raul::URI&   subject,
	                const Raul::URI&   key);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType { NO_ERROR, NOT_FOUND };
	enum {
		NONE,
		PORT_VALUE
	} _special_type;

	Raul::URI             _uri;
	Raul::URI             _key;
	Raul::Atom            _value;
	Shared::ResourceImpl* _resource;
	bool                  _is_meta;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_REQUESTMETADATA_HPP
