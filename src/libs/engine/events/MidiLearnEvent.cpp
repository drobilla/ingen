/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "MidiLearnEvent.h"
#include "Responder.h"
#include "Ingen.h"
#include "ObjectStore.h"
#include "Node.h"
#include "MidiControlNode.h"
#include "ClientBroadcaster.h"

namespace Ingen {


// MidiLearnResponseEvent

void
MidiLearnResponseEvent::post_process()
{
	Ingen::instance().client_broadcaster()->send_control_change(m_port_path, m_value);
}



// MidiLearnEvent

MidiLearnEvent::MidiLearnEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& node_path)
: QueuedEvent(responder, timestamp),
  m_node_path(node_path),
  m_node(NULL),
  m_response_event(NULL)
{
}


void
MidiLearnEvent::pre_process()
{
	m_node = Ingen::instance().object_store()->find_node(m_node_path);
	m_response_event = new MidiLearnResponseEvent(m_node_path + "/Controller_Number", _time_stamp);
	
	QueuedEvent::pre_process();
}


void
MidiLearnEvent::execute(SampleCount offset)
{	
	QueuedEvent::execute(offset);
	
	// FIXME: this isn't very good at all.
	if (m_node != NULL && m_node->plugin()->type() == Plugin::Internal
			&& m_node->plugin()->plug_label() == "midi_control_in") {
			((MidiControlNode*)m_node)->learn(m_response_event);
	}
}


void
MidiLearnEvent::post_process()
{
	if (m_node != NULL) {
		_responder->respond_ok();
	} else {
		string msg = "Did not find node '";
		msg.append(m_node_path).append("' for MIDI learn.");
		_responder->respond_error(msg);
	}
}


} // namespace Ingen


