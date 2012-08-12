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

#ifndef INGEN_INTERNALS_CONTROLLER_HPP
#define INGEN_INTERNALS_CONTROLLER_HPP

#include <string>
#include "NodeImpl.hpp"

namespace Ingen {
namespace Server {

class InputPort;
class OutputPort;
class InternalPlugin;

namespace Internals {

/** MIDI control input node.
 *
 * Creating one of these nodes is how a user makes "MIDI Bindings".  Note that
 * this node will always be monophonic, the poly parameter is ignored.
 *
 * \ingroup engine
 */
class ControllerNode : public NodeImpl
{
public:
	ControllerNode(InternalPlugin*     plugin,
	               BufferFactory&      bufs,
	               const Raul::Symbol& symbol,
	               bool                polyphonic,
	               PatchImpl*          parent,
	               SampleRate          srate);

	void process(ProcessContext& context);

	void control(ProcessContext& context, uint8_t control_num, uint8_t val, FrameTime time);

	void learn() { _learning = true; }

	static InternalPlugin* internal_plugin(URIs& uris);

private:
	InputPort*  _midi_in_port;
	InputPort*  _param_port;
	InputPort*  _log_port;
	InputPort*  _min_port;
	InputPort*  _max_port;
	OutputPort* _audio_port;
	bool        _learning;
};

} // namespace Server
} // namespace Ingen
} // namespace Internals

#endif // INGEN_INTERNALS_CONTROLLER_HPP
