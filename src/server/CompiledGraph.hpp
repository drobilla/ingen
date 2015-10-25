/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_COMPILEDGRAPH_HPP
#define INGEN_ENGINE_COMPILEDGRAPH_HPP

#include <vector>
#include <list>

#include "raul/Maid.hpp"
#include "raul/Noncopyable.hpp"

namespace Ingen {
namespace Server {

class BlockImpl;

/** All information required about a block to execute it in an audio thread.
 */
class CompiledBlock {
public:
	CompiledBlock(BlockImpl* b) : _block(b) {}

	BlockImpl* block() const { return _block; }

private:
	BlockImpl* _block;
};

/** A graph ``compiled'' into a flat structure with the correct order so
 * the audio thread(s) can execute it without threading problems (since
 * the preprocessor thread modifies the graph).
 *
 * The blocks contained here are sorted in the order they must be executed.
 * The parallel processing algorithm guarantees no block will be executed
 * before its providers, using this order as well as semaphores.
 */
class CompiledGraph : public std::vector<CompiledBlock>
                    , public Raul::Maid::Disposable
                    , public Raul::Noncopyable
{
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_COMPILEDGRAPH_HPP
