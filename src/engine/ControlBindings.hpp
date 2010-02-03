/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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

namespace Ingen {

class Engine;
class ProcessContext;
class EventBuffer;
class PortImpl;

class ControlBindings {
public:
	enum Type {
		MIDI_BENDER,
		MIDI_CC,
		MIDI_RPN,
		MIDI_NRPN,
		MIDI_CHANNEL_PRESSURE,
		MIDI_NOTE
	};

	struct Key {
		Key(Type t, int16_t n=0) : type(t), num(n) {}
		inline bool operator<(const Key& other) const {
			return (type == other.type) ? (num < other.num) : (type < other.type);
		}
		Type    type;
		int16_t num;
	};

	typedef std::map<Key, PortImpl*> Bindings;

	ControlBindings(Engine& engine, SharedPtr<Shared::LV2URIMap> map)
		: _engine(engine)
		, _map(map)
		, _learn_port(NULL)
		, _bindings(new Bindings())
	{}

	void learn(PortImpl* port);
	void update_port(ProcessContext& context, PortImpl* port);
	void process(ProcessContext& context, EventBuffer* buffer);

	/** Remove all bindings for @a path or children of @a path.
	 * The caller must safely drop the returned reference in the
	 * post-processing thread after at least one process thread has run.
	 */
	SharedPtr<Bindings> remove(const Raul::Path& path);

private:
	Engine&                      _engine;
	SharedPtr<Shared::LV2URIMap> _map;
	PortImpl*                    _learn_port;

	void set_port_value(ProcessContext& context, PortImpl* port, Type type, int16_t value);
	bool bind(ProcessContext& context, Type type, int16_t num=0);

	SharedPtr<Bindings> _bindings;
};

} // namespace Ingen

#endif // INGEN_ENGINE_CONTROLBINDINGS_HPP
