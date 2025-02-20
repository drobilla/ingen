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
#include "types.hpp"

#include <raul/Semaphore.hpp>

#include <boost/intrusive/options.hpp>
#include <boost/intrusive/slist.hpp>
#include <portaudio.h>

#include <atomic>
#include <cstdint>
#include <memory>

namespace ingen::server {

class Engine;
class FrameTimer;
class RunContext;

class PortAudioDriver : public Driver
{
public:
	explicit PortAudioDriver(Engine& engine);
	~PortAudioDriver() override;

	bool attach();

	bool activate() override;
	void deactivate() override;

	EnginePort* create_port(DuplexPort* graph_port) override;
	EnginePort* get_port(const raul::Path& path) override;

	void rename_port(const raul::Path& old_path, const raul::Path& new_path) override;
	void port_property(const raul::Path& path, const URI& uri, const Atom& value) override;
	void add_port(RunContext& ctx, EnginePort* port) override;
	void remove_port(RunContext& ctx, EnginePort* port) override;
	void register_port(EnginePort& port) override;
	void unregister_port(EnginePort& port) override;

	void append_time_events(RunContext& ctx, Buffer& buffer) override {}

	SampleCount frame_time() const override;

	int real_time_priority() override { return 80; }

	SampleCount    block_length() const override { return _block_length; }
	uint32_t       seq_size()     const override { return _seq_size; }
	SampleCount    sample_rate()  const override { return _sample_rate; }

private:
	friend class PortAudioPort;

	static int pa_process_cb(const void*                     inputs,
	                         void*                           outputs,
	                         unsigned long                   nframes,
	                         const PaStreamCallbackTimeInfo* time,
	                         PaStreamCallbackFlags           flags,
	                         void*                           handle)
	{
		return static_cast<PortAudioDriver*>(handle)->process_cb(
		    inputs, outputs, nframes, time, flags);
	}

	int process_cb(const void*                     inputs,
	               void*                           outputs,
	               unsigned long                   nframes,
	               const PaStreamCallbackTimeInfo* time,
	               PaStreamCallbackFlags           flags);

	void pre_process_port(RunContext&  ctx,
	                      EnginePort*  port,
	                      const void*  inputs,
	                      void*        outputs);

	void post_process_port(RunContext& ctx,
	                       EnginePort* port,
	                       const void* inputs,
	                       void*       outputs);

protected:
	using Ports = boost::intrusive::slist<EnginePort,
	                                      boost::intrusive::cache_last<true>>;

	Engine&                     _engine;
	Ports                       _ports;
	PaStreamParameters          _inputParameters;
	PaStreamParameters          _outputParameters;
	raul::Semaphore             _sem{0U};
	std::unique_ptr<FrameTimer> _timer;
	PaStream*                   _stream{nullptr};
	uint32_t                    _seq_size{4096U};
	uint32_t                    _block_length;
	uint32_t                    _sample_rate{48000U};
	uint32_t                    _n_inputs{0U};
	uint32_t                    _n_outputs{0U};
	std::atomic<bool>           _flag{false};
	bool                        _is_activated{false};
};

} // namespace ingen::server

#endif // INGEN_ENGINE_PORTAUDIODRIVER_HPP
