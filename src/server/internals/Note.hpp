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

#ifndef INGEN_INTERNALS_NOTE_HPP
#define INGEN_INTERNALS_NOTE_HPP

#include "InternalBlock.hpp"
#include "types.hpp"

#include "raul/Array.hpp"
#include "raul/Maid.hpp"

#include <cstdint>

namespace raul {
class Symbol;
} // namespace raul

namespace ingen {

class URIs;

namespace server {

class BufferFactory;
class GraphImpl;
class InputPort;
class InternalPlugin;
class OutputPort;
class RunContext;

namespace internals {

/** MIDI note input block.
 *
 * For pitched instruments like keyboard, etc.
 *
 * \ingroup engine
 */
class NoteNode : public InternalBlock
{
public:
	NoteNode(InternalPlugin*     plugin,
	         BufferFactory&      bufs,
	         const raul::Symbol& symbol,
	         bool                polyphonic,
	         GraphImpl*          parent,
	         SampleRate          srate);

	bool prepare_poly(BufferFactory& bufs, uint32_t poly) override;
	bool apply_poly(RunContext& ctx, uint32_t poly) override;

	void run(RunContext& ctx) override;

	void note_on(RunContext& ctx, uint8_t note_num, uint8_t velocity, FrameTime time);
	void note_off(RunContext& ctx, uint8_t note_num, FrameTime time);
	void all_notes_off(RunContext& ctx, FrameTime time);

	void sustain_on(RunContext& ctx, FrameTime time);
	void sustain_off(RunContext& ctx, FrameTime time);

	void bend(RunContext& ctx, FrameTime time, float amount);
	void note_pressure(RunContext& ctx, FrameTime time, uint8_t note_num, float amount);
	void channel_pressure(RunContext& ctx, FrameTime time, float amount);

	static InternalPlugin* internal_plugin(URIs& uris);

private:
	/** Key, one for each key on the keyboard */
	struct Key {
		enum class State { OFF, ON_ASSIGNED, ON_UNASSIGNED };

		State       state = State::OFF;
		uint32_t    voice = 0;
		SampleCount time  = 0;
	};

	/** Voice, one of these always exists for each voice */
	struct Voice {
		enum class State { FREE, ACTIVE, HOLDING };

		State       state = State::FREE;
		uint8_t     note  = 0;
		SampleCount time  = 0;
	};

	using Voices = raul::Array<Voice>;

	void free_voice(RunContext& ctx, uint32_t voice, FrameTime time);

	raul::managed_ptr<Voices> _voices;
	raul::managed_ptr<Voices> _prepared_voices;

	Key  _keys[128];
	bool _sustain;  ///< Whether or not hold pedal is depressed

	InputPort*  _midi_in_port;
	OutputPort* _freq_port;
	OutputPort* _num_port;
	OutputPort* _vel_port;
	OutputPort* _gate_port;
	OutputPort* _trig_port;
	OutputPort* _bend_port;
	OutputPort* _pressure_port;
};

} // namespace internals
} // namespace server
} // namespace ingen

#endif // INGEN_INTERNALS_NOTE_HPP
