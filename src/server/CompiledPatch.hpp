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

#ifndef INGEN_ENGINE_COMPILEDPATCH_HPP
#define INGEN_ENGINE_COMPILEDPATCH_HPP

#include <vector>
#include <list>

#include <boost/utility.hpp>

#include "raul/Deletable.hpp"
#include "raul/List.hpp"

namespace Ingen {
namespace Server {

class ConnectionImpl;
class NodeImpl;

/** All information required about a node to execute it in an audio thread.
 */
class CompiledNode {
public:
	CompiledNode(NodeImpl* n, size_t np, const std::list<NodeImpl*>& d)
		: _node(n), _n_providers(np)
	{
		// Copy to a vector for maximum iteration speed and cache optimization
		// (Need to take a copy anyway)

		_dependants.reserve(d.size());
		for (std::list<NodeImpl*>::const_iterator i = d.begin(); i != d.end(); ++i)
			_dependants.push_back(*i);
	}

	NodeImpl*                     node()        const { return _node; }
	size_t                        n_providers() const { return _n_providers; }
	const std::vector<NodeImpl*>& dependants()  const { return _dependants; }

private:
	NodeImpl*              _node;
	size_t                 _n_providers; ///< Number of input ready signals to trigger run
	std::vector<NodeImpl*> _dependants; ///< Nodes this one's output ports are connected to
};

/** A patch ``compiled'' into a flat structure with the correct order so
 * the audio thread(s) can execute it without threading problems (since
 * the preprocessor thread modifies the graph).
 *
 * The nodes contained here are sorted in the order they must be executed.
 * The parallel processing algorithm guarantees no node will be executed
 * before its providers, using this order as well as semaphores.
 */
class CompiledPatch : public std::vector<CompiledNode>
                    , public Raul::Deletable
                    , public boost::noncopyable
{
public:
	typedef std::vector<ConnectionImpl*> QueuedConnections;

	/** All (audio context => other context) connections */
	std::vector<ConnectionImpl*> queued_connections;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_COMPILEDPATCH_HPP
