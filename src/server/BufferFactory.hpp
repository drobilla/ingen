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

#include <atomic>
#include <map>
#include <mutex>

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/URIs.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "raul/RingBuffer.hpp"

#include "BufferRef.hpp"
#include "PortType.hpp"
#include "types.hpp"

namespace Ingen {

class URIs;

namespace Server {

class Engine;

class INGEN_API BufferFactory {
public:
	BufferFactory(Engine& engine, URIs& uris);
	~BufferFactory();

	static uint32_t audio_buffer_size(SampleCount nframes);
	uint32_t        default_size(LV2_URID type) const;

	BufferRef get_buffer(LV2_URID type,
	                     LV2_URID value_type,
	                     uint32_t capacity,
	                     bool     real_time,
	                     bool     force_create = false);

	BufferRef silent_buffer();

	void set_block_length(SampleCount block_length);
	void set_seq_size(uint32_t seq_size) { _seq_size = seq_size; }

	Forge&  forge();
	URIs&   uris()   { return _uris; }
	Engine& engine() { return _engine; }

private:
	friend class Buffer;
	void recycle(Buffer* buf);

	BufferRef create(LV2_URID type, LV2_URID value_type, uint32_t capacity=0);

	inline std::atomic<Buffer*>& free_list(LV2_URID type) {
		if (type == _uris.atom_Float) {
			return _free_control;
		} else if (type == _uris.atom_Sound) {
			return _free_audio;
		} else if (type == _uris.atom_Sequence) {
			return _free_sequence;
		} else {
			return _free_object;
		}
	}

	void free_list(Buffer* head);

	std::atomic<Buffer*> _free_audio;
	std::atomic<Buffer*> _free_control;
	std::atomic<Buffer*> _free_sequence;
	std::atomic<Buffer*> _free_object;

	std::mutex  _mutex;
	Engine&     _engine;
	URIs&       _uris;
	uint32_t    _seq_size;

	BufferRef _silent_buffer;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_BUFFERFACTORY_HPP
