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

#ifndef INGEN_ENGINE_INTERNALBLOCK_HPP
#define INGEN_ENGINE_INTERNALBLOCK_HPP

#include "BlockImpl.hpp"

namespace ingen {
namespace server {

/** An internal Block implemented inside Ingen.
 *
 * \ingroup engine
 */
class InternalBlock : public BlockImpl
{
public:
	InternalBlock(PluginImpl*         plugin,
	              const Raul::Symbol& symbol,
	              bool                poly,
	              GraphImpl*          parent,
	              SampleRate          rate);

	BlockImpl* duplicate(Engine&             engine,
	                     const Raul::Symbol& symbol,
	                     GraphImpl*          parent) override;

	void pre_process(RunContext& context) override;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_BLOCKIMPL_HPP
