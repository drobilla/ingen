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

#ifndef INGEN_ENGINE_PROCESSCONTEXT_HPP
#define INGEN_ENGINE_PROCESSCONTEXT_HPP

#include <vector>

#include "Context.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class ProcessSlave;

/** Context of a process() call (the audio context).
 * \ingroup engine
 */
class ProcessContext : public Context
{
public:
	explicit ProcessContext(Engine& engine);

	typedef std::vector<ProcessSlave*> Slaves;

	const Slaves& slaves() const { return _slaves; }
	Slaves&       slaves()       { return _slaves; }

	void activate(uint32_t parallelism, bool sched_rt);

private:
	Slaves _slaves;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PROCESSCONTEXT_HPP
