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

#ifndef INGEN_INTERNALS_CONTROLLER_HPP
#define INGEN_INTERNALS_CONTROLLER_HPP

#include "InternalBlock.hpp"
#include "types.hpp"

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

/** MIDI control input block.
 *
 * Creating one of these nodes is how a user makes "MIDI Bindings".  Note that
 * this node will always be monophonic, the poly parameter is ignored.
 *
 * \ingroup engine
 */
class ControllerNode : public InternalBlock
{
public:
	ControllerNode(InternalPlugin*      plugin,
	               BufferFactory&       bufs,
	               const raul::Symbol&  symbol,
	               bool                 polyphonic,
	               GraphImpl*           parent,
	               SampleRate           srate);

	void run(RunContext& ctx) override;

	void learn() override { _learning = true; }

	static InternalPlugin* internal_plugin(URIs& uris);

private:
	bool control(const RunContext& ctx,
	             uint8_t           control_num,
	             uint8_t           val,
	             FrameTime         time);

	InputPort*  _midi_in_port;
	OutputPort* _midi_out_port;
	InputPort*  _param_port;
	InputPort*  _log_port;
	InputPort*  _min_port;
	InputPort*  _max_port;
	OutputPort* _audio_port;
	bool        _learning{false};
};

} // namespace internals
} // namespace server
} // namespace ingen

#endif // INGEN_INTERNALS_CONTROLLER_HPP
