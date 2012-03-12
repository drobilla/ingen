/* This file is part of Ingen.
 * Copyright 2010-2011 David Robillard <http://drobilla.net>
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

#include "ingen/shared/Configuration.hpp"

using namespace Raul;

namespace Ingen {
namespace Shared {

Configuration::Configuration(Raul::Forge* forge)
	: Raul::Configuration(forge,
	"A realtime modular audio processor.",
	"Ingen is a flexible modular system that be used in various ways.\n"
	"The engine can run as a stand-alone server controlled via a network protocol\n"
	"(e.g. OSC), or internal to another process (e.g. the GUI).  The GUI, or other\n"
	"clients, can communicate with the engine via any supported protocol, or host the\n"
	"engine in the same process.  Many clients can connect to an engine at once.\n\n"
	"Examples:\n"
	"  ingen -e                    # Run an engine, listen for OSC\n"
	"  ingen -g                    # Run a GUI, connect via OSC\n"
	"  ingen -eg                   # Run an engine and a GUI in one process\n"
	"  ingen -eg patch.ttl         # Run an engine and a GUI and load a patch file\n"
	"  ingen -eg patch.ingen       # Run an engine and a GUI and load a patch bundle")
{
	add("client-port", 'C', "Client OSC port", Atom::INT, forge->make());
	add("connect",     'c', "Connect to engine URI", Atom::STRING, forge->make("osc.udp://localhost:16180"));
	add("engine",      'e', "Run (JACK) engine", Atom::BOOL, forge->make(false));
	add("engine-port", 'E', "Engine listen port", Atom::INT, forge->make(16180));
	add("gui",         'g', "Launch the GTK graphical interface", Atom::BOOL, forge->make(false));
	add("help",        'h', "Print this help message", Atom::BOOL, forge->make(false));
	add("jack-client", 'n', "JACK client name", Atom::STRING, forge->make("ingen"));
	add("jack-server", 's', "JACK server name", Atom::STRING, forge->make(""));
	add("uuid",        'u', "JACK session UUID", Atom::STRING, forge->make(""));
	add("load",        'l', "Load patch", Atom::STRING, forge->make());
	add("packet-size", 'k', "Maximum UDP packet size", Atom::INT, forge->make(4096));
	add("parallelism", 'p', "Number of concurrent process threads", Atom::INT, forge->make(1));
	add("path",        'L', "Target path for loaded patch", Atom::STRING, forge->make());
	add("queue-size",  'q', "Event queue size", Atom::INT, forge->make(4096));
	add("run",         'r', "Run script", Atom::STRING, forge->make());
}

} // namespace Shared
} // namespace Ingen
