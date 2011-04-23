/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_DISCONNECT_HPP
#define INGEN_EVENTS_DISCONNECT_HPP

#include "raul/Path.hpp"
#include "QueuedEvent.hpp"
#include "types.hpp"
#include "PatchImpl.hpp"
#include "BufferFactory.hpp"

namespace Raul {
	template <typename T> class ListNode;
	template <typename T> class Array;
}

namespace Ingen {
namespace Server {

class CompiledPatch;
class InputPort;
class OutputPort;
class PortImpl;

namespace Events {

/** Make a Connection between two Ports.
 *
 * \ingroup engine
 */
class Disconnect : public QueuedEvent
{
public:
	Disconnect(
			Engine&            engine,
			SharedPtr<Request> request,
			SampleCount        timestamp,
			const Raul::Path&  src_port_path,
			const Raul::Path&  dst_port_path);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

	class Impl {
	public:
		Impl(Engine&     e,
		     PatchImpl*  patch,
		     OutputPort* s,
		     InputPort*  d);

		bool execute(ProcessContext& context, bool set_dst_buffers);

		InputPort* dst_port() { return _dst_input_port; }

	private:
		Engine&                          _engine;
		OutputPort*                      _src_output_port;
		InputPort*                       _dst_input_port;
		PatchImpl*                       _patch;
		SharedPtr<ConnectionImpl>        _connection;
		Raul::Array<BufferFactory::Ref>* _buffers;
	};

private:
	enum ErrorType {
		NO_ERROR,
		PARENT_PATCH_DIFFERENT,
		PORT_NOT_FOUND,
		TYPE_MISMATCH,
		NOT_CONNECTED,
		PARENTS_NOT_FOUND,
		CONNECTION_NOT_FOUND
	};

	const Raul::Path _src_port_path;
	const Raul::Path _dst_port_path;

	PatchImpl* _patch;
	PortImpl*  _src_port;
	PortImpl*  _dst_port;

	Impl*          _impl;
	CompiledPatch* _compiled_patch;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_DISCONNECT_HPP