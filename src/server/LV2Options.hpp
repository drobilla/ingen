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

#ifndef INGEN_ENGINE_LV2OPTIONS_HPP
#define INGEN_ENGINE_LV2OPTIONS_HPP

#include "ingen/LV2Features.hpp"
#include "ingen/URIs.hpp"
#include "lv2/core/lv2.h"
#include "lv2/options/options.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace ingen {

class Node;
class World;

namespace server {

class LV2Options : public ingen::LV2Features::Feature {
public:
	explicit LV2Options(const URIs& uris)
		: _uris(uris)
	{}

	void set(int32_t sample_rate, int32_t block_length, int32_t seq_size) {
		_sample_rate  = sample_rate;
		_block_length = block_length;
		_seq_size     = seq_size;
	}

	const char* uri() const override { return LV2_OPTIONS__options; }

	std::shared_ptr<LV2_Feature> feature(World& w, Node* n) override {
		const LV2_Options_Option options[] = {
			{ LV2_OPTIONS_INSTANCE, 0, _uris.bufsz_minBlockLength,
			  sizeof(int32_t), _uris.atom_Int, &_block_length },
			{ LV2_OPTIONS_INSTANCE, 0, _uris.bufsz_maxBlockLength,
			  sizeof(int32_t), _uris.atom_Int, &_block_length },
			{ LV2_OPTIONS_INSTANCE, 0, _uris.bufsz_sequenceSize,
			  sizeof(int32_t), _uris.atom_Int, &_seq_size },
			{ LV2_OPTIONS_INSTANCE, 0, _uris.param_sampleRate,
			  sizeof(int32_t), _uris.atom_Int, &_sample_rate },
			{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr }
		};

		auto* f = static_cast<LV2_Feature*>(malloc(sizeof(LV2_Feature)));
		f->URI  = LV2_OPTIONS__options;
		f->data = malloc(sizeof(options));
		memcpy(f->data, options, sizeof(options));
		return std::shared_ptr<LV2_Feature>(f, &free_feature);
	}

private:
	const URIs& _uris;
	int32_t     _sample_rate  = 0;
	int32_t     _block_length = 0;
	int32_t     _seq_size     = 0;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_LV2OPTIONS_HPP
