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

#include <cassert>
#include "shared/LV2URIMap.hpp"
#include "internals/Note.hpp"
#include "internals/Trigger.hpp"
#include "internals/Controller.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "InternalPlugin.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Internals;

InternalPlugin::InternalPlugin(const std::string& uri, const std::string& symbol)
	: PluginImpl(Plugin::Internal, uri)
	, _symbol(symbol)
{
	const LV2URIMap& uris = Shared::LV2URIMap::instance();
	set_property(uris.rdf_type, uris.ingen_Internal);
}


NodeImpl*
InternalPlugin::instantiate(BufferFactory&    bufs,
                            const string&     name,
                            bool              polyphonic,
                            Ingen::PatchImpl* parent,
                            Engine&           engine)
{
	assert(_type == Internal);

	const SampleCount srate = engine.driver()->sample_rate();

	const string uri_str = uri().str();
	if (uri_str == NS_INTERNALS "Note") {
		return new NoteNode(bufs, name, polyphonic, parent, srate);
	} else if (uri_str == NS_INTERNALS "Trigger") {
		return new TriggerNode(bufs, name, polyphonic, parent, srate);
	} else if (uri_str == NS_INTERNALS "Controller") {
		return new ControllerNode(bufs, name, polyphonic, parent, srate);
	} else {
		return NULL;
	}
}


} // namespace Ingen
