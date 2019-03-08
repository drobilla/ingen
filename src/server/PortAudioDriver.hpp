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

#include "Driver.hpp"
#include "EnginePort.hpp"
#include "ingen_config.h"
#include "types.hpp"

#include "ingen/URI.hpp"
#include "lv2/atom/forge.h"
#include "raul/Semaphore.hpp"

#include <portaudio.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace Raul { class Path; }

namespace ingen {

class Atom;

namespace server {

class Buffer;
class DuplexPort;
class Engine;
class FrameTimer;
class RunContext;

class PortAudioDriver : public Driver
{
public:
	explicit PortAudioDriver(Engine& engine);
	~PortAudioDriver();

	bool attach();

	bool activate() override;
	void deactivate() override;

	EnginePort* create_port(DuplexPort* graph_port) override;
	EnginePort* get_port(const Raul::Path& path) override;

	void rename_port(const Raul::Path& old_path, const Raul::Path& new_path) override;
	void port_property(const Raul::Path& path, const URI& uri, const Atom& value) override;
	void add_port(RunContext& context, EnginePort* port) override;
	void remove_port(RunContext& context, EnginePort* port) override;
	void register_port(EnginePort& port) override;
	void unregister_port(EnginePort& port) override;

	void append_time_events(RunContext& context, Buffer& buffer) override {}

	SampleCount frame_time() const override;

	int real_time_priority() override { return 80; }

	SampleCount    block_length() const override { return _block_length; }
	size_t         seq_size()     const override { return _seq_size; }
	SampleCount    sample_rate()  const override { return _sample_rate; }

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
