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

#ifndef INGEN_ENGINE_ARC_IMPL_HPP
#define INGEN_ENGINE_ARC_IMPL_HPP

// IWYU pragma: no_include "raul/Path.hpp"

#include "BufferRef.hpp"

#include "ingen/Arc.hpp"
#include "raul/Noncopyable.hpp"

#include <boost/intrusive/slist_hook.hpp>

#include <cstdint>

namespace raul {
class Path; // IWYU pragma: keep
} // namespace raul

namespace ingen {
namespace server {

class InputPort;
class PortImpl;
class RunContext;

/** Represents a single inbound connection for an InputPort.
 *
 * This can be a group of ports (coming from a polyphonic Block) or
 * a single Port.  This class exists basically as an abstraction of mixing
 * down polyphonic inputs, so InputPort can just deal with mixing down
 * multiple connections (oblivious to the polyphonic situation of the
 * connection itself).
 *
 * This is stored in an intrusive slist in InputPort.
 *
 * \ingroup engine
 */
class ArcImpl
		: private raul::Noncopyable
		, public  Arc
		, public  boost::intrusive::slist_base_hook<>
{
public:
	ArcImpl(PortImpl* tail, PortImpl* head);
	~ArcImpl() override;

	PortImpl* tail() const { return _tail; }
	PortImpl* head() const { return _head; }

	const raul::Path& tail_path() const override;
	const raul::Path& head_path() const override;

	/** Get the buffer for a particular voice.
	 * An Arc is smart - it knows the destination port requesting the
	 * buffer, and will return accordingly (e.g. the same buffer for every
	 * voice in a mono->poly arc).
	 */
	BufferRef buffer(const RunContext& ctx, uint32_t voice) const;

	/** Whether this arc must mix down voices into a local buffer */
	bool must_mix() const;

	static bool can_connect(const PortImpl* src, const InputPort* dst);

protected:
	PortImpl* const _tail;
	PortImpl* const _head;
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_ARC_IMPL_HPP
