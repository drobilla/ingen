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

#ifndef INGEN_ENGINE_PORTAUDIODRIVER_HPP
#define INGEN_ENGINE_PORTAUDIODRIVER_HPP

#include "ingen_config.h"

#include <atomic>
#include <memory>
#include <string>

#include <portaudio.h>

#include "raul/Semaphore.hpp"

#include "lv2/atom/forge.h"

#include "Driver.hpp"
#include "EnginePort.hpp"

namespace Raul { class Path; }

namespace ingen {
namespace server {

class DuplexPort;
class Engine;
class GraphImpl;
class PortAudioDriver;
class PortImpl;
class FrameTimer;

class PortAudioDriver : public Driver
{
public:
	explicit PortAudioDriver(Engine& engine);
	~PortAudioDriver();

	bool attach();

	bool activate();
	void deactivate();

	EnginePort* create_port(DuplexPort* graph_port);
	EnginePort* get_port(const Raul::Path& path);

	void rename_port(const Raul::Path& old_path, const Raul::Path& new_path);
	void port_property(const Raul::Path& path, const URI& uri, const Atom& value);
	void add_port(RunContext& context, EnginePort* port);
	void remove_port(RunContext& context, EnginePort* port);
	void register_port(EnginePort& port);
	void unregister_port(EnginePort& port);

	void append_time_events(RunContext& context, Buffer& buffer) {}

	SampleCount frame_time() const;

	int real_time_priority() { return 80; }

	SampleCount    block_length() const { return _block_length; }
	size_t         seq_size()     const { return _seq_size; }
	SampleCount    sample_rate()  const { return _sample_rate; }

private:
	friend class PortAudioPort;

	inline static int
	pa_process_cb(const void*                     inputs,
	              void*                           outputs,
	              unsigned long                   nframes,
	              const PaStreamCallbackTimeInfo* time,
	              PaStreamCallbackFlags           flags,
	              void*                           handle) {
		return ((PortAudioDriver*)handle)->process_cb(
			inputs, outputs, nframes, time, flags);
	}

	int process_cb(const void*                     inputs,
	               void*                           outputs,
	               unsigned long                   nframes,
	               const PaStreamCallbackTimeInfo* time,
	               PaStreamCallbackFlags           flags);

	void pre_process_port(RunContext&  context,
	                      EnginePort*  port,
	                      const void*  inputs,
	                      void*        outputs);

	void post_process_port(RunContext& context,
	                       EnginePort* port,
	                       const void* inputs,
	                       void*       outputs);

protected:
	typedef boost::intrusive::slist<EnginePort,
	                                boost::intrusive::cache_last<true>
	                                > Ports;

	Engine&                     _engine;
	Ports                       _ports;
	PaStreamParameters          _inputParameters;
	PaStreamParameters          _outputParameters;
	Raul::Semaphore             _sem;
	std::unique_ptr<FrameTimer> _timer;
	PaStream*                   _stream;
	size_t                      _seq_size;
	uint32_t                    _block_length;
	uint32_t                    _sample_rate;
	uint32_t                    _n_inputs;
	uint32_t                    _n_outputs;
	std::atomic<bool>           _flag;
	bool                        _is_activated;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_PORTAUDIODRIVER_HPP
