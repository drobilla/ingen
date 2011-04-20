/* This file is part of Ingen.
 * Copyright 2009-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_BUFFERFACTORY_HPP
#define INGEN_ENGINE_BUFFERFACTORY_HPP

#include <map>
#include <boost/intrusive_ptr.hpp>
#include "ingen/PortType.hpp"
#include "glibmm/thread.h"
#include "raul/RingBuffer.hpp"
#include "raul/AtomicPtr.hpp"
#include "types.hpp"

namespace Ingen {

namespace Shared { class LV2URIMap; }

namespace Server {

class Engine;
class Buffer;

class BufferFactory {
public:
	BufferFactory(Engine&                             engine,
	              SharedPtr<Ingen::Shared::LV2URIMap> uris);

	~BufferFactory();

	typedef boost::intrusive_ptr<Buffer> Ref;

	static size_t audio_buffer_size(SampleCount nframes);
	size_t        default_buffer_size(PortType type);

	Ref get(PortType type, size_t size=0, bool force_create=false);

	Ref silent_buffer() { return _silent_buffer; }

	void set_block_length(SampleCount block_length);

	Ingen::Shared::LV2URIMap& uris() { assert(_uris); return *_uris.get(); }

private:
	friend class Buffer;
	void recycle(Buffer* buf);

	Ref create(PortType type, size_t size=0);

	inline Raul::AtomicPtr<Buffer>& free_list(PortType type) {
		switch (type.symbol()) {
		case PortType::AUDIO:   return _free_audio;
		case PortType::CONTROL: return _free_control;
		case PortType::EVENTS:  return _free_event;
		case PortType::VALUE:
		case PortType::MESSAGE: return _free_object;
		default: throw;
		}
	}

	void free_list(Buffer* head);

	Raul::AtomicPtr<Buffer> _free_audio;
	Raul::AtomicPtr<Buffer> _free_control;
	Raul::AtomicPtr<Buffer> _free_event;
	Raul::AtomicPtr<Buffer> _free_object;

	Glib::Mutex                         _mutex;
	Engine&                             _engine;
	SharedPtr<Ingen::Shared::LV2URIMap> _uris;

	Ref _silent_buffer;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BUFFERFACTORY_HPP
