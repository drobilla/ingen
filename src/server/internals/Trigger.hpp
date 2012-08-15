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

#ifndef INGEN_INTERNALS_TRIGGER_HPP
#define INGEN_INTERNALS_TRIGGER_HPP

#include "NodeImpl.hpp"

namespace Ingen {
namespace Server {

class InputPort;
class OutputPort;
class InternalPlugin;

namespace Internals {

/** MIDI trigger input node.
 *
 * Just has a gate,  for drums etc.  A control port is used to select
 * which note number is responded to.
 *
 * Note that this node is always monophonic, the poly parameter is ignored.
 * (Should that change?)
 *
 * \ingroup engine
 */
class TriggerNode : public NodeImpl
{
public:
	TriggerNode(InternalPlugin*     plugin,
	            BufferFactory&      bufs,
	            const Raul::Symbol& symbol,
	            bool                polyphonic,
	            PatchImpl*          parent,
	            SampleRate          srate);

	void process(ProcessContext& context);

	void note_on(ProcessContext& context, uint8_t note_num, uint8_t velocity, FrameTime time);
	void note_off(ProcessContext& context, uint8_t note_num, FrameTime time);

	void learn() { _learning = true; }

	static InternalPlugin* internal_plugin(URIs& uris);

private:
	bool _learning;

	InputPort*  _midi_in_port;
	InputPort*  _note_port;
	OutputPort* _gate_port;
	OutputPort* _trig_port;
	OutputPort* _vel_port;
};

} // namespace Server
} // namespace Ingen
} // namespace Internals

#endif // INGEN_INTERNALS_TRIGGER_HPP
