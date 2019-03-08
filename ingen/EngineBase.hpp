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

#ifndef INGEN_ENGINEBASE_HPP
#define INGEN_ENGINEBASE_HPP

#include "ingen/ingen.h"
#include "ingen/types.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace ingen {

class Interface;

/**
   The audio engine which executes the graph.

   @ingroup Ingen
*/
class INGEN_API EngineBase
{
public:
	virtual ~EngineBase() = default;

	/**
	   Initialise the engine for local use (e.g. without a Jack driver).
	   @param sample_rate Audio sampling rate in Hz.
	   @param block_length Audio block length (i.e. buffer size) in frames.
	   @param seq_size Sequence buffer size in bytes.
	*/
	virtual void init(double   sample_rate,
	                  uint32_t block_length,
	                  size_t   seq_size) = 0;

	/**
	   Return true iff the engine and driver supports dynamic ports.

	   This returns false in situations where top level ports can not be
	   created once the driver is running, which is the case for most
	   environments outside Jack.
	*/
	virtual bool supports_dynamic_ports() const = 0;

	/**
	   Activate the engine.
	*/
	virtual bool activate() = 0;

	/**
	   Deactivate the engine.
	*/
	virtual void deactivate() = 0;

	/**
	   Begin listening on network sockets.
	*/
	virtual void listen() = 0;

	/**
	   Return true iff events are waiting to be processed.
	*/
	virtual bool pending_events() const = 0;

	/**
	   Flush any pending events.

	   This function is only safe to call in sequential contexts, and runs both
	   process thread and main iterations in lock-step.

	   @param sleep_ms Interval in milliseconds to sleep between each block.
	*/
	virtual void flush_events(const std::chrono::milliseconds& sleep_ms) = 0;

	/**
	   Advance audio time by the given number of frames.
	*/
	virtual void advance(uint32_t nframes) = 0;

	/**
	   Locate to a given audio position.
	*/
	virtual void locate(uint32_t start, uint32_t sample_count) = 0;

	/**
	   Process audio for `sample_count` frames.

	   If the return value is non-zero, events have been processed and are
	   awaiting to be finalised (including responding and announcing any changes
	   to clients) via a call to main_iteration().

	   @return The number of events processed.
	*/
	virtual unsigned run(uint32_t sample_count) = 0;

	/**
	   Indicate that a quit is desired.

	   This function simply sets a flag which affects the return value of
	   main_iteration, it does not actually force the engine to stop running or
	   block.  The code driving the engine is responsible for stopping and
	   cleaning up when main_iteration returns false.
	*/
	virtual void quit() = 0;

	/**
	   Run a single iteration of the main context.

	   The main context post-processes events and performs housekeeping duties
	   like collecting garbage.  This should be called regularly, e.g. a few
	   times per second.  The return value indicates whether execution should
	   continue; i.e. if false is returned, a quit has been requested and the
	   caller should cease calling main_iteration() and stop the engine.
	*/
	virtual bool main_iteration() = 0;

	/**
	   Register a client to receive updates about engine changes.
	*/
	virtual void register_client(SPtr<Interface> client) = 0;

	/**
	   Unregister a client.
	*/
	virtual bool unregister_client(SPtr<Interface> client) = 0;
};

} // namespace ingen

#endif // INGEN_ENGINEBASE_HPP
