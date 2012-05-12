/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_BUFFERFACTORY_HPP
#define INGEN_ENGINE_BUFFERFACTORY_HPP

#include <map>

#undef nil
#include <glibmm/thread.h>

#include "raul/Atom.hpp"
#include "raul/AtomicPtr.hpp"
#include "raul/RingBuffer.hpp"
#include "raul/SharedPtr.hpp"
#include "ingen/shared/Forge.hpp"
#include "ingen/shared/URIs.hpp"

#include "Buffer.hpp"
#include "BufferRef.hpp"
#include "PortType.hpp"
#include "types.hpp"

namespace Ingen {

namespace Shared { class URIs; }

namespace Server {

class Context;
class Engine;

class BufferFactory {
public:
	BufferFactory(Engine& engine, Shared::URIs& uris);
	~BufferFactory();

	static uint32_t audio_buffer_size(SampleCount nframes);
	uint32_t        default_buffer_size(LV2_URID type);

	BufferRef get(Context& context,
	              LV2_URID type,
	              uint32_t capacity,
	              bool     force_create = false);

	BufferRef silent_buffer();

	void set_block_length(SampleCount block_length);

	Shared::Forge& forge();
	Shared::URIs&  uris()   { return _uris; }
	Engine&        engine() { return _engine; }

private:
	friend class Buffer;
	void recycle(Buffer* buf);

	BufferRef create(LV2_URID type, uint32_t capacity=0);

	inline Raul::AtomicPtr<Buffer>& free_list(LV2_URID type) {
		if (type == _uris.atom_Float) {
			return _free_control;
		} else if (type == _uris.atom_Sound) {
			return _free_audio;
		} else {
			return _free_object;
		}
	}

	void free_list(Buffer* head);

	Raul::AtomicPtr<Buffer> _free_audio;
	Raul::AtomicPtr<Buffer> _free_control;
	Raul::AtomicPtr<Buffer> _free_object;

	Glib::Mutex   _mutex;
	Engine&       _engine;
	Shared::URIs& _uris;

	BufferRef _silent_buffer;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BUFFERFACTORY_HPP
