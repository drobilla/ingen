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

#ifndef INGEN_ENGINE_LV2OPTIONS_HPP
#define INGEN_ENGINE_LV2OPTIONS_HPP

#include "ingen/LV2Features.hpp"
#include "lv2/lv2plug.in/ns/ext/options/options.h"

#include "BlockImpl.hpp"
#include "BufferFactory.hpp"
#include "Driver.hpp"
#include "Engine.hpp"

namespace Ingen {
namespace Server {

struct LV2Options : public Ingen::LV2Features::Feature {
	explicit LV2Options(Engine& engine)
		: _block_length(0)
		, _seq_size(0)
	{
		set(engine);
	}

	static void delete_feature(LV2_Feature* feature) {
		free(feature->data);
		free(feature);
	}

	void set(Engine& engine) {
		if (engine.driver()) {
			_block_length = engine.driver()->block_length();
			_seq_size = engine.buffer_factory()->default_size(
				engine.world()->uris().atom_Sequence);
		}
	}

	const char* uri() const { return LV2_OPTIONS__options; }

	SharedPtr<LV2_Feature> feature(World* w, Node* n) {
		BlockImpl* block = dynamic_cast<BlockImpl*>(n);
		if (!block) {
			return SharedPtr<LV2_Feature>();
		}
		Engine& engine = block->parent_graph()->engine();
		URIs&   uris   = engine.world()->uris();
		const LV2_Options_Option options[] = {
			{ LV2_OPTIONS_INSTANCE, 0, uris.bufsz_minBlockLength,
			  sizeof(int32_t), uris.atom_Int, &_block_length },
			{ LV2_OPTIONS_INSTANCE, 0, uris.bufsz_maxBlockLength,
			  sizeof(int32_t), uris.atom_Int, &_block_length },
			{ LV2_OPTIONS_INSTANCE, 0, uris.bufsz_sequenceSize,
			  sizeof(int32_t), uris.atom_Int, &_seq_size },
			{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
		};

		LV2_Feature* f = (LV2_Feature*)malloc(sizeof(LV2_Feature));
		f->URI  = LV2_OPTIONS__options;
		f->data = malloc(sizeof(options));
		memcpy(f->data, options, sizeof(options));
		return SharedPtr<LV2_Feature>(f, &delete_feature);
	}

private:
	int32_t _block_length;
	int32_t _seq_size;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2OPTIONS_HPP
