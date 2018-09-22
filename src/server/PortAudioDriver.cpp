/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen_config.h"

#include <cstdlib>
#include <string>

#include <portaudio.h>

#include "ingen/Configuration.hpp"
#include "ingen/LV2Features.hpp"
#include "ingen/Log.hpp"
#include "ingen/URIMap.hpp"
#include "ingen/World.hpp"
#include "lv2/atom/util.h"

#include "Buffer.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "PortAudioDriver.hpp"
#include "PortImpl.hpp"
#include "FrameTimer.hpp"
#include "ThreadManager.hpp"
#include "util.hpp"

namespace Ingen {
namespace Server {

static bool
pa_error(const char* msg, PaError err)
{
	fprintf(stderr, "error: %s (%s)\n", msg, Pa_GetErrorText(err));
	Pa_Terminate();
	return false;
}

PortAudioDriver::PortAudioDriver(Engine& engine)
	: _engine(engine)
	, _sem(0)
	, _stream(nullptr)
	, _seq_size(4096)
	, _block_length(engine.world()->conf().option("buffer-size").get<int32_t>())
	, _sample_rate(48000)
	, _n_inputs(0)
	, _n_outputs(0)
	, _flag(false)
	, _is_activated(false)
{
}

PortAudioDriver::~PortAudioDriver()
{
	deactivate();
	_ports.clear_and_dispose([](EnginePort* p) { delete p; });
}

bool
PortAudioDriver::attach()
{
	PaError st = paNoError;
	if ((st = Pa_Initialize())) {
		return pa_error("Failed to initialize audio system", st);
	}

	// Get default input and output devices
	_inputParameters.device  = Pa_GetDefaultInputDevice();
	_outputParameters.device = Pa_GetDefaultOutputDevice();
	if (_inputParameters.device == paNoDevice) {
		return pa_error("No default input device", paDeviceUnavailable);
	} else if (_outputParameters.device == paNoDevice) {
		return pa_error("No default output device", paDeviceUnavailable);
	}

	const PaDeviceInfo* in_dev = Pa_GetDeviceInfo(_inputParameters.device);

	/* TODO: It looks like it is somehow actually impossible to request the
	   best/native buffer size then retrieve what it actually is with
	   PortAudio.  How such a glaring useless flaw exists in such a widespread
	   library is beyond me... */

	_sample_rate = in_dev->defaultSampleRate;

	_timer = std::unique_ptr<FrameTimer>(
		new FrameTimer(_block_length, _sample_rate));

	return true;
}

bool
PortAudioDriver::activate()
{
	const PaDeviceInfo* in_dev  = Pa_GetDeviceInfo(_inputParameters.device);
	const PaDeviceInfo* out_dev = Pa_GetDeviceInfo(_outputParameters.device);

	// Count number of input and output audio ports/channels
	_inputParameters.channelCount  = 0;
	_outputParameters.channelCount = 0;
	for (const auto& port : _ports) {
		if (port.graph_port()->is_a(PortType::AUDIO)) {
			if (port.graph_port()->is_input()) {
				++_inputParameters.channelCount;
			} else if (port.graph_port()->is_output()) {
				++_outputParameters.channelCount;
			}
		}
	}

	// Configure audio format
	_inputParameters.sampleFormat               = paFloat32|paNonInterleaved;
	_inputParameters.suggestedLatency           = in_dev->defaultLowInputLatency;
	_inputParameters.hostApiSpecificStreamInfo  = nullptr;
	_outputParameters.sampleFormat              = paFloat32|paNonInterleaved;
	_outputParameters.suggestedLatency          = out_dev->defaultLowOutputLatency;
	_outputParameters.hostApiSpecificStreamInfo = nullptr;

	// Open stream
	PaError st = paNoError;
	if ((st = Pa_OpenStream(
		     &_stream,
		     _inputParameters.channelCount ? &_inputParameters : nullptr,
		     _outputParameters.channelCount ? &_outputParameters : nullptr,
		     in_dev->defaultSampleRate,
		     _block_length, // paFramesPerBufferUnspecified, // FIXME: ?
		     0,
		     pa_process_cb,
		     this))) {
		return pa_error("Failed to open audio stream", st);
	}

	_is_activated = true;
	if ((st = Pa_StartStream(_stream))) {
		return pa_error("Error starting audio stream", st);
	}

	return true;
}

void
PortAudioDriver::deactivate()
{
	Pa_Terminate();
}

SampleCount
PortAudioDriver::frame_time() const
{
	return _timer->frame_time(_engine.current_time()) + _engine.block_length();
}

EnginePort*
PortAudioDriver::get_port(const Raul::Path& path)
{
	for (auto& p : _ports) {
		if (p.graph_port()->path() == path) {
			return &p;
		}
	}

	return nullptr;
}

void
PortAudioDriver::add_port(RunContext& context, EnginePort* port)
{
	_ports.push_back(*port);
}

void
PortAudioDriver::remove_port(RunContext& context, EnginePort* port)
{
	_ports.erase(_ports.iterator_to(*port));
}

void
PortAudioDriver::register_port(EnginePort& port)
{
}

void
PortAudioDriver::unregister_port(EnginePort& port)
{
}

void
PortAudioDriver::rename_port(const Raul::Path& old_path,
                             const Raul::Path& new_path)
{
}

void
PortAudioDriver::port_property(const Raul::Path& path,
                               const URI&        uri,
                               const Atom&       value)
{
}

EnginePort*
PortAudioDriver::create_port(DuplexPort* graph_port)
{
	EnginePort* eport = nullptr;
	if (graph_port->is_a(PortType::AUDIO) || graph_port->is_a(PortType::CV)) {
		// Audio buffer port, use Jack buffer directly
		eport = new EnginePort(graph_port);
		graph_port->set_is_driver_port(*_engine.buffer_factory());
	} else if (graph_port->is_a(PortType::ATOM) &&
	           graph_port->buffer_type() == _engine.world()->uris().atom_Sequence) {
		// Sequence port, make Jack port but use internal LV2 format buffer
		eport = new EnginePort(graph_port);
	}

	if (graph_port->is_a(PortType::AUDIO)) {
		if (graph_port->is_input()) {
			eport->set_driver_index(_n_inputs++);
		} else {
			eport->set_driver_index(_n_outputs++);
		}
	}

	if (eport) {
		register_port(*eport);
	}

	return eport;
}

void
PortAudioDriver::pre_process_port(RunContext& context,
                                  EnginePort* port,
                                  const void* inputs,
                                  void*       outputs)
{
	if (!port->graph_port()->is_a(PortType::AUDIO)) {
		return;
	}

	if (port->is_input()) {
		port->set_buffer(((float**)inputs)[port->driver_index()]);
	} else {
		port->set_buffer(((float**)outputs)[port->driver_index()]);
		memset(port->buffer(), 0, _block_length * sizeof(float));
	}

	port->graph_port()->set_driver_buffer(
		port->buffer(), _block_length * sizeof(float));
}

void
PortAudioDriver::post_process_port(RunContext& context,
                                   EnginePort* port,
                                   const void* inputs,
                                   void*       outputs)
{
}

int
PortAudioDriver::process_cb(const void*                     inputs,
                            void*                           outputs,
                            unsigned long                   nframes,
                            const PaStreamCallbackTimeInfo* time,
                            PaStreamCallbackFlags           flags)
{
	_engine.advance(nframes);
	_timer->update(_engine.current_time(), _engine.run_context().start());

	// Read input
	for (auto& p : _ports) {
		pre_process_port(_engine.run_context(), &p, inputs, outputs);
	}

	// Process
	_engine.run(nframes);

	// Write output
	for (auto& p : _ports) {
		post_process_port(_engine.run_context(), &p, inputs, outputs);
	}

	return 0;
}

} // namespace Server
} // namespace Ingen
