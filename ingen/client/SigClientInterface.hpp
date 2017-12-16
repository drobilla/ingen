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

#ifndef INGEN_CLIENT_SIGCLIENTINTERFACE_HPP
#define INGEN_CLIENT_SIGCLIENTINTERFACE_HPP

#include <stdint.h>

#include <string>

#include "raul/Path.hpp"

#include "ingen/Interface.hpp"
#include "ingen/client/signal.hpp"
#include "ingen/ingen.h"

namespace Ingen {
namespace Client {

/** A LibSigC++ signal emitting interface for clients to use.
 *
 * This simply emits a signal for every event that comes from the engine.
 * For a higher level model based view of the engine, use ClientStore.
 *
 * The signals here match the calls to ClientInterface exactly.  See the
 * documentation for ClientInterface for meanings of signal parameters.
 *
 * @ingroup IngenClient
 */
class INGEN_API SigClientInterface : public Ingen::Interface,
                                     public INGEN_TRACKABLE
{
public:
	SigClientInterface() {}

	Raul::URI uri() const { return Raul::URI("ingen:/clients/sig"); }

	INGEN_SIGNAL(message, void, Message)

	/** Fire pending signals.  Only does anything on derived classes (that may queue) */
	virtual bool emit_signals() { return false; }

protected:
	void message(const Message& msg) override {
		_signal_message(msg);
	}
};

} // namespace Client
} // namespace Ingen

#endif
