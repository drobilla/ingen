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

#ifndef INGEN_ENGINE_PORTIMPL_HPP
#define INGEN_ENGINE_PORTIMPL_HPP

#include <cstdlib>

#include "ingen/Atom.hpp"
#include "raul/Array.hpp"

#include "BufferRef.hpp"
#include "NodeImpl.hpp"
#include "PortType.hpp"
#include "RunContext.hpp"
#include "types.hpp"

namespace Raul { class Maid; }

namespace ingen {
namespace server {

class BlockImpl;
class BufferFactory;
class RunContext;

/** A port (input or output) on a Block.
 *
 * The base implementation here is general and/or for output ports (which are
 * simplest), InputPort and DuplexPort override functions to provide
 * specialized behaviour where necessary.
 *
 * \ingroup engine
 */
class PortImpl : public NodeImpl
{
public:
	struct SetState {
		enum class State {
			/// Partially set, first cycle: AAAAA => AAABB.
			HALF_SET_CYCLE_1,

			/// Partially set, second cycle: AAABB => BBBBB.
			HALF_SET_CYCLE_2,

			/// Fully set, first cycle (clear events if necessary).
			SET_CYCLE_1,

			/// Fully set, second cycle and onwards (done).
			SET
		};

		SetState() : state(State::SET), value(0), time(0) {}

		void set(const RunContext& context, FrameTime t, Sample v) {
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
		Voice() : buffer(nullptr) {}

		SetState  set_state;
		BufferRef buffer;
	};

	typedef Raul::Array<Voice> Voices;

	PortImpl(BufferFactory&      bufs,
	         BlockImpl*          block,
	         const Raul::Symbol& name,
	         uint32_t            index,
	         uint32_t            poly,
	         PortType            type,
	         LV2_URID            buffer_type,
	         const Atom&         value,
	         size_t              buffer_size = 0,
	         bool                is_output = true);

	virtual GraphType graph_type() const { return GraphType::PORT; }

	/** A port's parent is always a block, so static cast should be safe */
	BlockImpl* parent_block() const { return (BlockImpl*)_parent; }

	/** Set the the voices (buffers) for this port in the audio thread. */
	void set_voices(RunContext& context, MPtr<Voices>&& voices);

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
	virtual bool apply_poly(RunContext& context, uint32_t poly);

	/** Return the number of arcs (pre-process thraed). */
	virtual size_t num_arcs() const { return 0; }

	const Atom& value() const { return _value; }
	void        set_value(const Atom& v) { _value = v; }

	const Atom& minimum() const { return _min; }
	const Atom& maximum() const { return _max; }

	/* The following two methods store the range in variables so it can be
	   accessed in the process thread, which is required for applying control
	   bindings from incoming MIDI data.
	*/
	void set_minimum(const Atom& min) { _min.set_rt(min); }
	void set_maximum(const Atom& max) { _max.set_rt(max); }

	inline BufferRef buffer(uint32_t voice) const {
		return _voices->at((_poly == 1) ? 0 : voice).buffer;
	}
	inline BufferRef prepared_buffer(uint32_t voice) const {
		return _prepared_voices->at(voice).buffer;
	}

	void update_set_state(const RunContext& context, uint32_t v);

	void set_voice_value(const RunContext& context,
	                     uint32_t          voice,
	                     FrameTime         time,
	                     Sample            value);

	void set_control_value(const RunContext& context,
	                       FrameTime         time,
	                       Sample            value);

	/** Prepare this port to use an external driver-provided buffer.
	 *
	 * This will avoid allocating a buffer for the port, instead the driver
	 * buffer is used directly.  This only makes sense for ports on the
	 * top-level graph, which are monophonic.  Non-real-time, must be called
	 * before using the port, followed by a call to set_driver_buffer() in the
	 * processing thread.
	 */
	virtual void set_is_driver_port(BufferFactory& bufs);

	bool is_driver_port() const { return _is_driver_port; }

	/** Called once per process cycle */
	virtual void pre_process(RunContext& context);
	virtual void pre_run(RunContext& context) {}
	virtual void post_process(RunContext& context);

	/** Clear/silence all buffers */
	virtual void clear_buffers(const RunContext& ctx);

	/** Claim and apply buffers in the real-time thread. */
	virtual bool setup_buffers(RunContext& ctx, BufferFactory& bufs, uint32_t poly);

	void activate(BufferFactory& bufs);
	void deactivate();

	/**
	   Inherit any properties from a connected neighbour.

	   This is used for Graph ports, so e.g. a control input has the range of
	   all the ports it is connected to.
	*/
	virtual void inherit_neighbour(const PortImpl* port,
	                               Properties&     remove,
	                               Properties&     add) {}

	virtual void connect_buffers(SampleCount offset=0);
	virtual void recycle_buffers();

	uint32_t index() const { return _index; }
	void set_index(RunContext&, uint32_t index) { _index = index; }

	inline bool is_a(PortType type) const { return _type == type; }

	bool has_value() const;

	PortType type()        const { return _type; }
	LV2_URID value_type()  const { return _value.is_valid() ? _value.type() : 0; }
	LV2_URID buffer_type() const { return _buffer_type; }

	bool supports(const URIs::Quark& value_type) const;

	size_t buffer_size() const { return _buffer_size; }

	uint32_t poly() const {
		return _poly;
	}
	uint32_t prepared_poly() const {
		return (_prepared_voices) ? _prepared_voices->size() : 1;
	}

	void set_buffer_size(RunContext& context, BufferFactory& bufs, size_t size);

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
	void monitor(RunContext& context, bool send_now=false);

	BufferFactory& bufs() const { return _bufs; }

	BufferRef value_buffer(uint32_t voice);

	BufferRef user_buffer(RunContext&) const { return _user_buffer; }
	void      set_user_buffer(RunContext&, BufferRef b) { _user_buffer = b; }

	/** Return offset of the first value change after `offset`. */
	virtual SampleCount next_value_offset(SampleCount offset,
	                                      SampleCount end) const;

	/** Update value buffer for `voice` to be current as of `offset`. */
	void update_values(SampleCount offset, uint32_t voice);

	void force_monitor_update() { _force_monitor_update = true; }

	void set_morphable(bool is_morph, bool is_auto_morph) {
		_is_morph      = is_morph;
		_is_auto_morph = is_auto_morph;
	}

	void set_type(PortType port_type, LV2_URID buffer_type);

	void cache_properties();

	bool is_input()       const { return !_is_output; }
	bool is_output()      const { return _is_output; }
	bool is_morph()       const { return _is_morph; }
	bool is_auto_morph()  const { return _is_auto_morph; }
	bool is_logarithmic() const { return _is_logarithmic; }
	bool is_sample_rate() const { return _is_sample_rate; }
	bool is_toggled()     const { return _is_toggled; }

protected:
	typedef BufferRef (BufferFactory::*GetFn)(LV2_URID, LV2_URID, uint32_t);

	/** Set `voices` as the buffers to be used for this port.
	 *
	 * This is real-time safe only if `get` is as well, use in the real-time
	 * thread should pass &BufferFactory::claim_buffer.
	 *
	 * @return true iff buffers are locally owned by the port
	 */
	virtual bool get_buffers(BufferFactory&      bufs,
	                         GetFn               get,
	                         const MPtr<Voices>& voices,
	                         uint32_t            poly,
	                         size_t              num_in_arcs) const;

 	BufferFactory&   _bufs;
	uint32_t         _index;
	uint32_t         _poly;
	uint32_t         _buffer_size;
	uint32_t         _frames_since_monitor;
	float            _monitor_value;
	float            _peak;
	PortType         _type;
	LV2_URID         _buffer_type;
	Atom             _value;
	Atom             _min;
	Atom             _max;
	MPtr<Voices>     _voices;
	MPtr<Voices>     _prepared_voices;
	BufferRef        _user_buffer;
	std::atomic_flag _connected_flag;
	bool             _monitored;
	bool             _force_monitor_update;
	bool             _is_morph;
	bool             _is_auto_morph;
	bool             _is_logarithmic;
	bool             _is_sample_rate;
	bool             _is_toggled;
	bool             _is_driver_port;
	bool             _is_output;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_PORTIMPL_HPP
