/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "InternalPlugin.hpp"

#include "Engine.hpp"
#include "internals/BlockDelay.hpp"
#include "internals/Controller.hpp"
#include "internals/Note.hpp"
#include "internals/Time.hpp"
#include "internals/Trigger.hpp"
#include "types.hpp"

#include "ingen/URIs.hpp"

#include <utility>

namespace ingen {
namespace server {

InternalPlugin::InternalPlugin(URIs& uris, const URI& uri, raul::Symbol symbol)
    : PluginImpl(uris, uris.ingen_Internal.urid_atom(), uri)
    , _symbol(std::move(symbol))
{
	set_property(uris.rdf_type, uris.ingen_Internal);
}

BlockImpl*
InternalPlugin::instantiate(BufferFactory&      bufs,
                            const raul::Symbol& symbol,
                            bool                polyphonic,
                            GraphImpl*          parent,
                            Engine&             engine,
                            const LilvState*)
{
	const SampleCount srate = engine.sample_rate();

	if (uri() == NS_INTERNALS "BlockDelay") {
		return new internals::BlockDelayNode(
		    this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Controller") {
		return new internals::ControllerNode(
		    this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Note") {
		return new internals::NoteNode(
		    this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Time") {
		return new internals::TimeNode(
		    this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Trigger") {
		return new internals::TriggerNode(
		    this, bufs, symbol, polyphonic, parent, srate);
	} else {
		return nullptr;
	}
}

} // namespace server
} // namespace ingen
