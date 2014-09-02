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

#ifndef INGEN_ENGINE_PORTIMPL_HPP
#define INGEN_ENGINE_PORTIMPL_HPP

#include <cstdlib>

#include "ingen/Atom.hpp"
#include "raul/Array.hpp"

#include "BufferRef.hpp"
#include "NodeImpl.hpp"
#include "PortType.hpp"
#include "ProcessContext.hpp"
#include "types.hpp"

namespace Raul { class Maid; }

namespace Ingen {
namespace Server {

class BlockImpl;
class BufferFactory;

/** A port (input or output) on a Block.
 *
 * \ingroup engine
 */
class PortImpl : public NodeImpl
{
public:
	struct SetState {
		enum class State { SET, HALF_SET_CYCLE_1, HALF_SET_CYCLE_2 };

		SetState() : state(State::SET), value(0), time(0) {}

		void set(const Context& context, FrameTime t, Sample v) {
			time  = t;
			value = v;
			state = (time == context.start()
			         ? State::SET
			         : State::HALF_SET_CYCLE_1);
		}

		State     state;  ///< State of buffer for setting control value
		Sample    value;  ///< Value currently being set
		FrameTime time;   ///< Time value was set
	};

	struct Voice {
		Voice() : buffer(NULL) {}

		SetState  set_state;
		BufferRef buffer;
	};

	~PortImpl();

	virtual GraphType graph_type() const { return GraphType::PORT; }

	/** A port's parent is always a block, so static cast should be safe */
	BlockImpl* parent_block() const { return (BlockImpl*)_parent; }

	/** Set the buffers array for this port.
	 *
	 * Audio thread.  Returned value must be freed by caller.
	 * \a buffers must be poly() long
	 */
	Raul::Array<Voice>* set_voices(ProcessContext&     context,
	                               Raul::Array<Voice>* voices);

	/** Prepare for a new (external) polyphony value.
	 *
	 * Preprocessor thread, poly is actually applied by apply_poly.
	 */
	virtual bool prepare_poly(BufferFactory& bufs, uint32_t poly);

	/** Apply a new polyphony value.
	 *
	 * Audio thread.
	 * \a poly Must be < the most recent value passed to prepare_poly.
	 */
	virtual bool apply_poly(
		ProcessContext& context, Raul::Maid& maid, uint32_t poly);

	const Atom& value() const { return _value; }
	void        set_value(const Atom& v) { _value = v; }

	const Atom& minimum() const { return _min; }
	const Atom& maximum() const { return _max; }

	/* The following two methods store the range in variables so it can be
	   accessed in the process thread, which is required for applying control
	   bindings from incoming MIDI data.
	*/
	void set_minimum(const Atom& min) { _min = min; }
	void set_maximum(const Atom& max) { _max = max; }

	inline BufferRef buffer(uint32_t voice) const {
		return _voices->at((_poly == 1) ? 0 : voice).buffer;
	}
	inline BufferRef prepared_buffer(uint32_t voice) const {
		return _prepared_voices->at(voice).buffer;
	}

	void update_set_state(Context& context, uint32_t voice);

	void set_voice_value(const Context& context,
	                     uint32_t       voice,
	                     FrameTime      time,
	                     Sample         value);

	void set_control_value(const Context& context,
	                       FrameTime      time,
	                       Sample         value);

	/** Called once per process cycle */
	virtual void pre_process(Context& context) = 0;
	virtual void pre_run(Context& context) {}
	virtual void post_process(Context& context) = 0;

	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers();

public:
	virtual bool get_buffers(BufferFactory&      bufs,
	                         Raul::Array<Voice>* voices,
	                         uint32_t            poly,
	                         bool                real_time) const = 0;

	void setup_buffers(BufferFactory& bufs, uint32_t poly, bool real_time) {
		get_buffers(bufs, _voices, poly, real_time);
	}

	void activate(BufferFactory& bufs);
	void deactivate();

	/**
	   Inherit any properties from a connected neighbour.

	   This is used for Graph ports, so e.g. a control input has the range of
	   all the ports it is connected to.
	*/
	virtual void inherit_neighbour(const PortImpl*       port,
	                               Resource::Properties& remove,
	                               Resource::Properties& add) {}

	virtual void connect_buffers(SampleCount offset=0);
	virtual void recycle_buffers();

	virtual bool is_input()  const = 0;
	virtual bool is_output() const = 0;

	uint32_t index() const { return _index; }

	inline bool is_a(PortType type) const { return _type == type; }

	bool has_value() const;

	PortType type()        const { return _type; }
	LV2_URID buffer_type() const { return _buffer_type; }

	bool supports(const Raul::URI& value_type) const;

	size_t buffer_size() const { return _buffer_size; }

	uint32_t poly() const {
		return _poly;
	}
	uint32_t prepared_poly() const {
		return (_prepared_voices) ? _prepared_voices->size() : 1;
	}

	void set_buffer_size(Context& context, BufferFactory& bufs, size_t size);

	/** Return true iff this port is explicitly monitored.
	 *
	 * This is used for plugin UIs which require monitoring for particular
	 * ports, even if the Ingen client has not requested broadcasting in
	 * general (e.g. for canvas animation).
	 */
	bool is_monitored() const { return _monitored; }

	/** Explicitly turn on monitoring for this port. */
	void enable_monitoring(bool monitored) { _monitored = monitored; }

	/** Monitor port value and broadcast to clients periodically. */
	void monitor(Context& context, bool send_now=false);

	void raise_set_by_user_flag() { _set_by_user = true; }

	BufferFactory& bufs() const { return _bufs; }

	BufferRef value_buffer(uint32_t voice);

	/** Return offset of the first value change after `offset`. */
	virtual SampleCount next_value_offset(SampleCount offset,
	                                      SampleCount end) const;

	/** Update value buffer for `voice` to be current as of `offset`. */
	virtual void update_values(SampleCount offset, uint32_t voice) = 0;

	void force_monitor_update() { _force_monitor_update = true; }

	void set_morphable(bool is_morph, bool is_auto_morph) {
		_is_morph      = is_morph;
		_is_auto_morph = is_auto_morph;
	}

	void set_type(PortType port_type, LV2_URID buffer_type);

	void cache_properties();

	bool is_morph()       const { return _is_morph; }
	bool is_auto_morph()  const { return _is_auto_morph; }
	bool is_logarithmic() const { return _is_logarithmic; }
	bool is_sample_rate() const { return _is_sample_rate; }
	bool is_toggled()     const { return _is_toggled; }

protected:
	PortImpl(BufferFactory&      bufs,
	         BlockImpl*          block,
	         const Raul::Symbol& name,
	         uint32_t            index,
	         uint32_t            poly,
	         PortType            type,
	         LV2_URID            buffer_type,
	         const Atom&         value,
	         size_t              buffer_size);

	BufferFactory&      _bufs;
	uint32_t            _index;
	uint32_t            _poly;
	uint32_t            _buffer_size;
	uint32_t            _frames_since_monitor;
	float               _monitor_value;
	float               _peak;
	PortType            _type;
	LV2_URID            _buffer_type;
	Atom                _value;
	Atom                _min;
	Atom                _max;
	Raul::Array<Voice>* _voices;
	Raul::Array<Voice>* _prepared_voices;
	bool                _monitored;
	bool                _force_monitor_update;
	bool                _set_by_user;
	bool                _is_morph;
	bool                _is_auto_morph;
	bool                _is_logarithmic;
	bool                _is_sample_rate;
	bool                _is_toggled;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PORTIMPL_HPP
