/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_CREATEPATCH_HPP
#define INGEN_EVENTS_CREATEPATCH_HPP

#include "Event.hpp"
#include "ingen/Resource.hpp"

namespace Ingen {
namespace Server {

class PatchImpl;
class CompiledPatch;

namespace Events {

/** Creates a new Patch.
 *
 * \ingroup engine
 */
class CreatePatch : public Event
{
public:
	CreatePatch(Engine&                     engine,
	            ClientInterface*            client,
	            int32_t                     id,
	            SampleCount                 timestamp,
	            const Raul::Path&           path,
	            int                         poly,
	            const Resource::Properties& properties);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	const Raul::Path _path;
	PatchImpl*       _patch;
	PatchImpl*       _parent;
	CompiledPatch*   _compiled_patch;
	int              _poly;

	Resource::Properties _properties;
};

} // namespace Server
} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_CREATEPATCH_HPP
