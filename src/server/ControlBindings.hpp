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

#ifndef INGEN_ENGINE_CONTROLBINDINGS_HPP
#define INGEN_ENGINE_CONTROLBINDINGS_HPP

#include "BufferRef.hpp"

#include "lv2/atom/forge.h"
#include "raul/Maid.hpp"

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/set_hook.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace raul {
class Path;
} // namespace raul

namespace boost {
namespace intrusive {

template <class Compare> struct compare;

} // namespace intrusive
} // namespace boost

namespace ingen {

class Atom;

namespace server {

class Buffer;
class Engine;
class RunContext;
class PortImpl;

class ControlBindings
{
public:
	enum class Type : uint16_t {
		NULL_CONTROL,
		MIDI_BENDER,
		MIDI_CC,
		MIDI_RPN,
		MIDI_NRPN,
		MIDI_CHANNEL_PRESSURE,
		MIDI_NOTE
	};

	struct Key {
		Key(Type t = Type::NULL_CONTROL, int16_t n = 0) noexcept
		    : type(t), num(n)
		{}

		inline bool operator<(const Key& other) const {
			return ((type < other.type) ||
			        (type == other.type && num < other.num));
		}

		inline bool operator==(const Key& other) const {
			return type == other.type && num == other.num;
		}

		inline bool operator!() const { return type == Type::NULL_CONTROL; }

		Type    type;
		int16_t num;
	};

	/** One binding of a controller to a port. */
	struct Binding : public boost::intrusive::set_base_hook<>,
	                 public raul::Maid::Disposable {
		Binding(Key k=Key(), PortImpl* p=nullptr) : key(k), port(p) {}

		inline bool operator<(const Binding& rhs) const { return key < rhs.key; }

		Key       key;
		PortImpl* port;
	};

	/** Comparator for bindings by key. */
	struct BindingLess {
		bool operator()(const Binding& lhs, const Binding& rhs) const {
			return lhs.key < rhs.key;
		}
	};

	explicit ControlBindings(Engine& engine);
	~ControlBindings();

	Key port_binding(PortImpl* port) const;
	Key binding_key(const Atom& binding) const;

	void start_learn(PortImpl* port);

	/** Set the binding for `port` to `binding` and take ownership of it. */
	bool set_port_binding(RunContext& ctx,
	                      PortImpl*   port,
	                      Binding*    binding,
	                      const Atom& value);

	void port_value_changed(RunContext& ctx,
	                        PortImpl*   port,
	                        Key         key,
	                        const Atom& value_atom);

	void pre_process(RunContext& ctx, Buffer* buffer);
	void post_process(RunContext& ctx, Buffer* buffer);

	/** Get all bindings for `path` or children of `path`. */
	void get_all(const raul::Path& path, std::vector<Binding*>& bindings);

	/** Remove a set of bindings from an earlier call to get_all(). */
	void remove(RunContext& ctx, const std::vector<Binding*>& bindings);

private:
	using Bindings =
	        boost::intrusive::multiset<Binding,
	                                   boost::intrusive::compare<BindingLess>>;

	static Key
	midi_event_key(uint16_t size, const uint8_t* buf, uint16_t& value);

	void set_port_value(RunContext& ctx,
	                    PortImpl*   port,
	                    Type        type,
	                    int16_t     value) const;

	bool finish_learn(RunContext& ctx, Key key);

	static float control_to_port_value(RunContext&     ctx,
	                                   const PortImpl* port,
	                                   Type            type,
	                                   int16_t         value);

	static int16_t port_value_to_control(RunContext& ctx,
	                                     PortImpl*   port,
	                                     Type        type,
	                                     const Atom& value_atom);

	Engine&                   _engine;
	std::atomic<Binding*>     _learn_binding;
	std::shared_ptr<Bindings> _bindings;
	BufferRef                 _feedback;
	LV2_Atom_Forge            _forge;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_CONTROLBINDINGS_HPP
