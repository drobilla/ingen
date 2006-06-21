/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MIDIMESSAGE_H
#define MIDIMESSAGE_H

namespace Om {


/** Midi Message (the type a MIDI port connects to).
 *
 * For the time being (ie while it's still possible) this is binary
 * compatible with jack_default_midi_event_t.  Don't mess that up without
 * dealing with all the repercussions (in the MidiDriver's).
 *
 * Note that with the current implementation one of these is NOT valid
 * across process cycles (since the buffer is just a pointer to the bufferr
 * given to us by Jack itself.  In other words, if one of these needs to be
 * stored, it needs to be deep copied.  Less copying anyway.. :/
 */
struct MidiMessage
{
public:
	MidiMessage() : time(0), size(0), buffer(NULL) {}
	
	// Really just to allow setting to zero for generic algos
	MidiMessage(const int& i) : time(0), size(0), buffer(NULL) {}

	samplecount    time;
    size_t         size;
    unsigned char* buffer;
};


} // namespace Om


#endif // MIDIMESSAGE_H
