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

#ifndef INGEN_ENGINE_CONTROLBINDINGS_HPP
#define INGEN_ENGINE_CONTROLBINDINGS_HPP

#include <map>
#include <stdint.h>

#include "ingen/Atom.hpp"
#include "ingen/types.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "raul/Path.hpp"

#include "BufferFactory.hpp"

namespace Ingen {
namespace Server {

class Engine;
class ProcessContext;
class PortImpl;

class ControlBindings {
public:
	enum class Type {
		NULL_CONTROL,
		MIDI_BENDER,
		MIDI_CC,
		MIDI_RPN,
		MIDI_NRPN,
		MIDI_CHANNEL_PRESSURE,
		MIDI_NOTE
	};

	struct Key {
		Key(Type t=Type::NULL_CONTROL, int16_t n=0) : type(t), num(n) {}
		inline bool operator<(const Key& other) const {
			return (type == other.type) ? (num < other.num) : (type < other.type);
		}
		inline operator bool() const { return type != Type::NULL_CONTROL; }
		Type    type;
		int16_t num;
	};

	typedef std::map<Key, PortImpl*> Bindings;

	explicit ControlBindings(Engine& engine);
	~ControlBindings();

	Key port_binding(PortImpl* port) const;
	Key binding_key(const Atom& binding) const;

	void learn(PortImpl* port);

	void port_binding_changed(ProcessContext&   context,
	                          PortImpl*         port,
	                          const Atom& binding);

	void port_value_changed(ProcessContext&   context,
	                        PortImpl*         port,
	                        Key               key,
	                        const Atom& value);

	void pre_process(ProcessContext& context, Buffer* control_in);
	void post_process(ProcessContext& context, Buffer* control_out);

	/** Remove all bindings for `path` or children of `path`.
	 * The caller must safely drop the returned reference in the
	 * post-processing thread after at least one process thread has run.
	 */
	SPtr<Bindings> remove(const Raul::Path& path);

	/** Remove binding for a particular port.
	 * The caller must safely drop the returned reference in the
	 * post-processing thread after at least one process thread has run.
	 */
	SPtr<Bindings> remove(PortImpl* port);

private:
	Key midi_event_key(uint16_t size, const uint8_t* buf, uint16_t& value);

	void set_port_value(ProcessContext& context,
	                    PortImpl*       port,
	                    Type            type,
	                    int16_t         value);

	bool bind(ProcessContext& context, Key key);

	Atom control_to_port_value(ProcessContext& context,
	                           const PortImpl* port,
	                           Type            type,
	                           int16_t         value) const;

	int16_t port_value_to_control(ProcessContext& context,
	                              PortImpl*       port,
	                              Type            type,
	                              const Atom&     value) const;

	Engine&        _engine;
	PortImpl*      _learn_port;
	SPtr<Bindings> _bindings;
	BufferRef      _feedback;
	LV2_Atom_Forge _forge;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_CONTROLBINDINGS_HPP
