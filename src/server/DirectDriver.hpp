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

#ifndef INGEN_ENGINE_DIRECT_DRIVER_HPP
#define INGEN_ENGINE_DIRECT_DRIVER_HPP

#include "Driver.hpp"
#include "Engine.hpp"

#include <boost/intrusive/slist.hpp>

namespace ingen {
namespace server {

/** Driver for running Ingen directly as a library.
 * \ingroup engine
 */
class DirectDriver : public Driver {
public:
	DirectDriver(Engine&     engine,
	             double      sample_rate,
	             SampleCount block_length,
	             size_t      seq_size)
		: _engine(engine)
		, _sample_rate(sample_rate)
		, _block_length(block_length)
		, _seq_size(seq_size)
	{}

	virtual ~DirectDriver() {
		_ports.clear_and_dispose([](EnginePort* p) { delete p; });
	}

	bool dynamic_ports() const override { return true; }

	EnginePort* create_port(DuplexPort* graph_port) override {
		return new EnginePort(graph_port);
	}

	EnginePort* get_port(const Raul::Path& path) override {
		for (auto& p : _ports) {
			if (p.graph_port()->path() == path) {
				return &p;
			}
		}

		return nullptr;
	}

	void add_port(RunContext&, EnginePort* port) override {
		_ports.push_back(*port);
	}

	void remove_port(RunContext&, EnginePort* port) override {
		_ports.erase(_ports.iterator_to(*port));
	}

	void rename_port(const Raul::Path& old_path,
	                 const Raul::Path& new_path) override {}

	void port_property(const Raul::Path& path,
	                   const URI&        uri,
	                   const Atom&       value) override {}

	void register_port(EnginePort& port) override {}
	void unregister_port(EnginePort& port) override {}

	SampleCount block_length() const override { return _block_length; }

	size_t seq_size() const override { return _seq_size; }

	SampleCount sample_rate() const override { return _sample_rate; }

	SampleCount frame_time() const override {
		return _engine.run_context().start();
	}

	void append_time_events(RunContext&, Buffer&) override {}

	int real_time_priority() override { return 60; }

private:
	using Ports = boost::intrusive::slist<EnginePort,
	                                      boost::intrusive::cache_last<true>>;

	Engine&     _engine;
	Ports       _ports;
	SampleCount _sample_rate;
	SampleCount _block_length;
	size_t      _seq_size;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_DIRECT_DRIVER_HPP
