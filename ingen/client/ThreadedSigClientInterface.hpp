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

#ifndef INGEN_CLIENT_THREADEDSIGCLIENTINTERFACE_HPP
#define INGEN_CLIENT_THREADEDSIGCLIENTINTERFACE_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#undef nil
#include <sigc++/sigc++.h>

#include "ingen/Atom.hpp"
#include "ingen/Interface.hpp"
#include "ingen/client/SigClientInterface.hpp"
#include "ingen/ingen.h"

/** Returns nothing and takes no parameters (because they have all been bound) */
typedef sigc::slot<void> Closure;

namespace Ingen {

class Interface;

namespace Client {

/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This emits signals (possibly) in a different thread than the ClientInterface
 * functions are called.  It must be explicitly driven with the emit_signals()
 * function, which fires all enqueued signals up until the present.  You can
 * use this in a GTK idle callback for receiving thread safe engine signals.
 *
 * @ingroup IngenClient
 */
class INGEN_API ThreadedSigClientInterface : public SigClientInterface
{
public:
	ThreadedSigClientInterface()
		: message_slot(_signal_message.make_slot())
	{}

	Raul::URI uri() const override {
		return Raul::URI("ingen:/clients/sig_queue");
	}

	void message(const Message& msg) override {
		std::lock_guard<std::mutex> lock(_mutex);
		_sigs.push_back(sigc::bind(message_slot, msg));
	}

	/** Process all queued events - Called from GTK thread to emit signals. */
	bool emit_signals() override {
		// Get pending signals
		std::vector<Closure> sigs;
		{
			std::lock_guard<std::mutex> lock(_mutex);
			std::swap(sigs, _sigs);
		}

		for (auto& ev : sigs) {
			ev();
			ev.disconnect();
		}

		return true;
	}

private:
	std::mutex           _mutex;
	std::vector<Closure> _sigs;

	using Graph = Resource::Graph;
	using Path  = Raul::Path;
	using URI   = Raul::URI;

	sigc::slot<void, Message> message_slot;
};

} // namespace Client
} // namespace Ingen

#endif
