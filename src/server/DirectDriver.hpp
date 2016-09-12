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

#ifndef INGEN_ENGINE_DIRECT_DRIVER_HPP
#define INGEN_ENGINE_DIRECT_DRIVER_HPP

#include <boost/intrusive/list.hpp>

#include "Driver.hpp"

namespace Ingen {
namespace Server {

/** Driver for running Ingen directly as a library.
 * \ingroup engine
 */
class DirectDriver : public Driver {
public:
	DirectDriver(double sample_rate, SampleCount block_length, size_t seq_size)
		: _sample_rate(sample_rate)
		, _block_length(block_length)
		, _seq_size(seq_size)
	{}

	virtual ~DirectDriver() {}

	virtual void activate() {}

	virtual void deactivate() {}

	virtual EnginePort* create_port(DuplexPort* graph_port) {
		return new EnginePort(graph_port);
	}

	virtual EnginePort* get_port(const Raul::Path& path) {
		for (auto& p : _ports) {
			if (p.graph_port()->path() == path) {
				return &p;
			}
		}

		return NULL;
	}

	virtual void add_port(RunContext& context, EnginePort* port) {
		_ports.push_back(*port);
	}

	virtual void remove_port(RunContext& context, EnginePort* port) {
		_ports.erase(_ports.iterator_to(*port));
	}

	virtual void rename_port(const Raul::Path& old_path,
	                         const Raul::Path& new_path) {}

	virtual void port_property(const Raul::Path& path,
	                           const Raul::URI&  uri,
	                           const Atom&       value) {}

	virtual void register_port(EnginePort& port) {}
	virtual void unregister_port(EnginePort& port) {}

	virtual SampleCount block_length() const { return _block_length; }

	virtual size_t seq_size() const { return _seq_size; }

	virtual SampleCount sample_rate() const { return _sample_rate; }

	virtual SampleCount frame_time() const { return 0; }

	virtual void append_time_events(RunContext& context,
	                                Buffer&     buffer) {}

	virtual int real_time_priority() { return 60; }

private:
	typedef boost::intrusive::list<EnginePort> Ports;

	Ports       _ports;
	SampleCount _sample_rate;
	SampleCount _block_length;
	size_t      _seq_size;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_DIRECT_DRIVER_HPP
