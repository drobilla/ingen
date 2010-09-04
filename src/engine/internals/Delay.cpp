/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include <cmath>
#include <limits.h>
#include "raul/log.hpp"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/midi_events.h"
#include "shared/LV2URIMap.hpp"
#include "internals/Delay.hpp"
#include "AudioBuffer.hpp"
#include "Driver.hpp"
#include "EventBuffer.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "PatchImpl.hpp"
#include "ProcessContext.hpp"
#include "ingen-config.h"
#include "util.hpp"

#define LOG(s) s << "[DelayNode] "

#define CALC_DELAY(delaytime) \
        (f_clamp (delaytime * (float)sample_rate, 1.0f, (float)(buffer_mask + 1)))

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Internals {

using namespace Shared;

static const float MAX_DELAY_SECONDS = 8.0f;

InternalPlugin* DelayNode::internal_plugin(Shared::LV2URIMap& uris) {
	return new InternalPlugin(uris, NS_INTERNALS "Delay", "delay");
}

DelayNode::DelayNode(
		InternalPlugin*    plugin,
		BufferFactory&     bufs,
		const std::string& path,
		bool               polyphonic,
		PatchImpl*         parent,
		SampleRate         srate)
	: NodeImpl(plugin, path, polyphonic, parent, srate)
	, _buffer(0)
	, _buffer_length(0)
	, _buffer_mask(0)
	, _write_phase(0)
{
	const LV2URIMap& uris = bufs.uris();
	_ports = new Raul::Array<PortImpl*>(3);

	const float default_delay = 1.0f;
	_last_delay_time = default_delay;
	_delay_samples = default_delay;

	_delay_port = new InputPort(bufs, this, "delay", 1, _polyphony, PortType::CONTROL, default_delay);
	_delay_port->set_property(uris.lv2_name, "Delay");
	_delay_port->set_property(uris.lv2_default, default_delay);
	_delay_port->set_property(uris.lv2_minimum, (float)(1.0/(double)srate));
	_delay_port->set_property(uris.lv2_maximum, MAX_DELAY_SECONDS);
	_ports->at(0) = _delay_port;

	_in_port = new InputPort(bufs, this, "in", 0, 1, PortType::AUDIO, 0.0f);
	_in_port->set_property(uris.lv2_name, "Input");
	_ports->at(1) = _in_port;

	_out_port = new OutputPort(bufs, this, "out", 0, 1, PortType::AUDIO, 0.0f);
	_out_port->set_property(uris.lv2_name, "Output");
	_ports->at(2) = _out_port;

	//_buffer = bufs.get(PortType::AUDIO, bufs.audio_buffer_size(buffer_length_frames), true);

}


DelayNode::~DelayNode()
{
	//_buffer.reset();
	free(_buffer);
}


void
DelayNode::activate(BufferFactory& bufs)
{
	NodeImpl::activate(bufs);
	const SampleCount min_size = MAX_DELAY_SECONDS * _srate;

	// Smallest power of two larger than min_size
	SampleCount size = 1;
	while (size < min_size)
		size <<= 1;

	_buffer = (float*)calloc(size, sizeof(float));
	_buffer_mask = size - 1;
	_buffer_length = size;
	//_buffer->clear();
	_write_phase = 0;
}


static inline float f_clamp(float x, float a, float b)
{
	const float x1 = fabs(x - a);
	const float x2 = fabs(x - b);

	x = x1 + a + b;
	x -= x2;
	x *= 0.5;

	return x;
}


static inline float cube_interp(const float fr, const float inm1, const float
                                in, const float inp1, const float inp2)
{
	return in + 0.5f * fr * (inp1 - inm1 +
	 fr * (4.0f * inp1 + 2.0f * inm1 - 5.0f * in - inp2 +
	 fr * (3.0f * (in - inp1) - inm1 + inp2)));
}


void
DelayNode::process(ProcessContext& context)
{
	AudioBuffer* const delay_buf = (AudioBuffer*)_delay_port->buffer(0).get();
	AudioBuffer* const in_buf    = (AudioBuffer*)_in_port->buffer(0).get();
	AudioBuffer* const out_buf   = (AudioBuffer*)_out_port->buffer(0).get();

	NodeImpl::pre_process(context);

	DelayNode* plugin_data = this;

	const float* const in            = in_buf->data();
	float* const       out           = out_buf->data();
	const float        delay_time    = delay_buf->data()[0];
	const uint32_t     buffer_mask   = plugin_data->_buffer_mask;
	const unsigned int sample_rate   = plugin_data->_srate;
	float              delay_samples = plugin_data->_delay_samples;
	long               write_phase   = plugin_data->_write_phase;
	const uint32_t     sample_count  = context.nframes();

	if (write_phase == 0) {
		_last_delay_time = delay_time;
		_delay_samples   = delay_samples = CALC_DELAY(delay_time);
	}

	if (delay_time == _last_delay_time) {
		const long  idelay_samples = (long)delay_samples;
		const float frac           = delay_samples - idelay_samples;

		for (uint32_t i = 0; i < sample_count; i++) {
			long read_phase = write_phase - (long)delay_samples;
			const float read = cube_interp(frac,
					buffer_at(read_phase - 1),
					buffer_at(read_phase),
					buffer_at(read_phase + 1),
					buffer_at(read_phase + 2));
			buffer_at(write_phase++) = in[i];
			out[i] = read;
		}
	} else {
		const float next_delay_samples  = CALC_DELAY(delay_time);
		const float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

		for (uint32_t i = 0; i < sample_count; i++) {
			delay_samples += delay_samples_slope;
			write_phase++;
			const long  read_phase     = write_phase - (long)delay_samples;
			const long  idelay_samples = (long)delay_samples;
			const float frac           = delay_samples - idelay_samples;
			const float read           = cube_interp(frac,
					buffer_at(read_phase - 1),
					buffer_at(read_phase),
					buffer_at(read_phase + 1),
					buffer_at(read_phase + 2));
			buffer_at(write_phase) = in[i];
			out[i] = read;
		}

		_last_delay_time = delay_time;
		_delay_samples   = delay_samples;
	}

	_write_phase = write_phase;

	NodeImpl::post_process(context);
}


} // namespace Internals
} // namespace Ingen

