/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "InternalBlock.hpp"

#include <cstdint>

namespace ingen {
namespace server {

class InputPort;
class OutputPort;
class InternalPlugin;

namespace internals {

/** MIDI trigger input block.
 *
 * Just has a gate, for drums etc.  A control port is used to select
 * which note number is responded to.
 *
 * Note that this block is always monophonic, the poly parameter is ignored.
 * (Should that change?)
 *
 * \ingroup engine
 */
class TriggerNode : public InternalBlock
{
public:
	TriggerNode(InternalPlugin*     plugin,
	            BufferFactory&      bufs,
	            const Raul::Symbol& symbol,
	            bool                polyphonic,
	            GraphImpl*          parent,
	            SampleRate          srate);

	void run(RunContext& context) override;

	bool note_on(RunContext& context, uint8_t note_num, uint8_t velocity, FrameTime time);
	bool note_off(RunContext& context, uint8_t note_num, FrameTime time);

	void learn() override { _learning = true; }

	static InternalPlugin* internal_plugin(URIs& uris);

private:
	bool _learning;

	InputPort*  _midi_in_port;
	OutputPort* _midi_out_port;
	InputPort*  _note_port;
	OutputPort* _gate_port;
	OutputPort* _trig_port;
	OutputPort* _vel_port;
};

} // namespace server
} // namespace ingen
} // namespace internals

#endif // INGEN_INTERNALS_TRIGGER_HPP
