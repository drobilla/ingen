/* This file is part of Ingen.
 * Copyright (C) 2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_CONTROLBINDINGS_HPP
#define INGEN_ENGINE_CONTROLBINDINGS_HPP

#include <stdint.h>
#include <map>
#include "raul/SharedPtr.hpp"
#include "raul/Path.hpp"
#include "shared/LV2URIMap.hpp"
#include "BufferFactory.hpp"

namespace Ingen {

class Engine;
class ProcessContext;
class EventBuffer;
class PortImpl;

class ControlBindings {
public:
	enum Type {
		NULL_CONTROL,
		MIDI_BENDER,
		MIDI_CC,
		MIDI_RPN,
		MIDI_NRPN,
		MIDI_CHANNEL_PRESSURE,
		MIDI_NOTE
	};

	struct Key {
		Key(Type t=NULL_CONTROL, int16_t n=0) : type(t), num(n) {}
		inline bool operator<(const Key& other) const {
			return (type == other.type) ? (num < other.num) : (type < other.type);
		}
		inline operator bool() const { return type != NULL_CONTROL; }
		Type    type;
		int16_t num;
	};

	typedef std::map<Key, PortImpl*> Bindings;

	ControlBindings(Engine& engine);
	~ControlBindings();

	void learn(PortImpl* port);

	void port_binding_changed(ProcessContext& context, PortImpl* port);
	void port_value_changed(ProcessContext& context, PortImpl* port);
	void pre_process(ProcessContext& context, EventBuffer* control_in);
	void post_process(ProcessContext& context, EventBuffer* control_out);

	/** Remove all bindings for @a path or children of @a path.
	 * The caller must safely drop the returned reference in the
	 * post-processing thread after at least one process thread has run.
	 */
	SharedPtr<Bindings> remove(const Raul::Path& path);

	/** Remove binding for a particular port.
	 * The caller must safely drop the returned reference in the
	 * post-processing thread after at least one process thread has run.
	 */
	SharedPtr<Bindings> remove(PortImpl* port);

private:
	Key port_binding(PortImpl* port);
	Key midi_event_key(uint16_t size, uint8_t* buf, uint16_t& value);

	void set_port_value(ProcessContext& context, PortImpl* port, Type type, int16_t value);
	bool bind(ProcessContext& context, Key key);

	Raul::Atom control_to_port_value(PortImpl* port, Type type, int16_t value);
	int16_t    port_value_to_control(PortImpl* port, Type type);

	Engine&   _engine;
	PortImpl* _learn_port;

	SharedPtr<Bindings> _bindings;
	EventBuffer*        _feedback;
};

} // namespace Ingen

#endif // INGEN_ENGINE_CONTROLBINDINGS_HPP
