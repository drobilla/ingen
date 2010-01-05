/* This file is part of Ingen.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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

#ifndef BUFFER_FACTORY_H
#define BUFFER_FACTORY_H

#include <map>
#include "interface/PortType.hpp"
#include "glibmm/thread.h"
#include "raul/RingBuffer.hpp"
#include "raul/AtomicPtr.hpp"

namespace Ingen {

using namespace Shared;

class Engine;

namespace Shared { class LV2URIMap; }

class BufferFactory {
public:
	BufferFactory(Engine& engine, SharedPtr<Shared::LV2URIMap> map);

	SharedPtr<Buffer> get(Shared::PortType type, size_t size=0, bool force_create=false);

private:
	friend class BufferDeleter;
	void recycle(Buffer* buf);

	SharedPtr<Buffer> create(Shared::PortType type, size_t size=0);

	inline Raul::AtomicPtr<Buffer>& free_list(Shared::PortType type) {
		switch (type.symbol()) {
		case PortType::AUDIO:   return _free_audio;
		case PortType::CONTROL: return _free_control;
		case PortType::EVENTS:  return _free_event;
		case PortType::VALUE:
		case PortType::MESSAGE: return _free_object;
		default: throw;
		}
	}

	Raul::AtomicPtr<Buffer> _free_audio;
	Raul::AtomicPtr<Buffer> _free_control;
	Raul::AtomicPtr<Buffer> _free_event;
	Raul::AtomicPtr<Buffer> _free_object;

	Glib::Mutex                  _mutex;
	Engine&                      _engine;
	SharedPtr<Shared::LV2URIMap> _map;
};

} // namespace Ingen

#endif // BUFFER_FACTORY_H
