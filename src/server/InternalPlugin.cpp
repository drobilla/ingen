/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/URIs.hpp"
#include "internals/Controller.hpp"
#include "internals/Delay.hpp"
#include "internals/Note.hpp"
#include "internals/Time.hpp"
#include "internals/Trigger.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "InternalPlugin.hpp"

using namespace std;

namespace Ingen {
namespace Server {

using namespace Internals;

InternalPlugin::InternalPlugin(URIs&               uris,
                               const Raul::URI&    uri,
                               const Raul::Symbol& symbol)
	: PluginImpl(uris, Plugin::Internal, uri)
	, _symbol(symbol)
{
	set_property(uris.rdf_type, uris.ingen_Internal);
}

BlockImpl*
InternalPlugin::instantiate(BufferFactory&      bufs,
                            const Raul::Symbol& symbol,
                            bool                polyphonic,
                            GraphImpl*          parent,
                            Engine&             engine)
{
	const SampleCount srate = engine.driver()->sample_rate();

	if (uri() == NS_INTERNALS "Controller") {
		return new ControllerNode(this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Delay") {
		return new DelayNode(this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Note") {
		return new NoteNode(this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Time") {
		return new TimeNode(this, bufs, symbol, polyphonic, parent, srate);
	} else if (uri() == NS_INTERNALS "Trigger") {
		return new TriggerNode(this, bufs, symbol, polyphonic, parent, srate);
	} else {
		return NULL;
	}
}

} // namespace Server
} // namespace Ingen
