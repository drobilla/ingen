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

#include "Plugin.h"
#include "MidiNoteNode.h"
#include "MidiTriggerNode.h"
#include "MidiControlNode.h"
#include "TransportNode.h"

namespace Ingen {

Node*
Plugin::instantiate(const string& name, size_t poly, Ingen::Patch* parent, SampleRate srate, size_t buffer_size)
{
	assert(_type == Internal);

	if (_uri == "ingen:note_node") {
		return new MidiNoteNode(name, poly, parent, srate, buffer_size);
	} else if (_uri == "ingen:trigger_node") {
		return new MidiTriggerNode(name, poly, parent, srate, buffer_size);
	} else if (_uri == "ingen:control_node") {
		return new MidiControlNode(name, poly, parent, srate, buffer_size);
	} else if (_uri == "ingen:transport_node") {
		return new TransportNode(name, poly, parent, srate, buffer_size);
	} else {
		return NULL;
	}
}


} // namespace Ingen
