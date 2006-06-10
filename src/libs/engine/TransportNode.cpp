/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "TransportNode.h"
#include <jack/transport.h>
#include "OutputPort.h"
#include "Plugin.h"
#include "JackAudioDriver.h"
#include "Port.h"
#include "util.h"
#include "Om.h"
#include "OmApp.h"
#include "PortInfo.h"

namespace Om {


TransportNode::TransportNode(const string& path, size_t poly, Patch* parent, samplerate srate, size_t buffer_size)
: InternalNode(path, 1, parent, srate, buffer_size)
{
	m_num_ports = 10;
	m_ports.alloc(m_num_ports);

	OutputPort<sample>* spb_port = new OutputPort<sample>(this, "Seconds per Beat", 0, 1,
		new PortInfo("Seconds per Beat", CONTROL, OUTPUT, 0, 0, 1), 1);
	m_ports.at(0) = spb_port;

	OutputPort<sample>* bpb_port = new OutputPort<sample>(this, "Beats per Bar", 1, 1,
		new PortInfo("Beats per Bar", CONTROL, OUTPUT, 0, 0, 1), 1);
	m_ports.at(1) = bpb_port;
	
	OutputPort<sample>* bar_port = new OutputPort<sample>(this, "Bar", 3, 1,
		new PortInfo("Bar", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(2) = bar_port;
	
	OutputPort<sample>* beat_port = new OutputPort<sample>(this, "Beat", 3, 1,
		new PortInfo("Beat", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(3) = beat_port;
	
	OutputPort<sample>* frame_port = new OutputPort<sample>(this, "Frame", 3, 1,
		new PortInfo("Frame", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(4) = frame_port;

	OutputPort<sample>* hour_port = new OutputPort<sample>(this, "Hour", 3, 1,
		new PortInfo("Hour", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(5) = hour_port;
	
	OutputPort<sample>* minute_port = new OutputPort<sample>(this, "Minute", 3, 1,
		new PortInfo("Minute", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(6) = minute_port;

	OutputPort<sample>* second_port = new OutputPort<sample>(this, "Second", 3, 1,
		new PortInfo("Second", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(7) = second_port;
	
	OutputPort<sample>* trg_port = new OutputPort<sample>(this, "Beat Tick", 2, 1,
		new PortInfo("Beat Tick", AUDIO, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(8) = trg_port;

	OutputPort<sample>* bar_trig_port = new OutputPort<sample>(this, "Bar Tick", 3, 1,
		new PortInfo("Bar Tick", AUDIO, OUTPUT, 0, 0, 1), buffer_size);
	m_ports.at(9) = bar_trig_port;

	m_plugin.type(Plugin::Internal);
	m_plugin.plug_label("transport");
	m_plugin.name("Om Transport Node (BROKEN)");
}


void
TransportNode::run(size_t nframes)
{
	NodeBase::run(nframes);
#if 0

	// FIXME: this will die horribly with any driver other than jack (in theory)
	const jack_position_t* const position = ((JackAudioDriver*)om->audio_driver())->position();
	jack_transport_state_t state = ((JackAudioDriver*)om->audio_driver())->transport_state();
	double bpm = position->beats_per_minute;
	float bpb = position->beats_per_bar;
	float spb = 60.0 / bpm;
	
	//cerr << "bpm = " << bpm << endl;
	//cerr << "spb = " << spb << endl; 
	
	if (position->valid & JackPositionBBT) {
		cerr << "bar: " << position->bar << endl;
		cerr << "beat: " << position->beat << endl;
		cerr << "tick: " << position->tick << endl;
	} else {
		cerr << "No BBT" << endl;
	}

	if (position->valid & JackBBTFrameOffset) {
		cerr << "bbt_offset: " << position->bbt_offset << endl;
	} else {
		cerr << "No BBT offset" << endl;
	}

	if (position->valid & JackPositionTimecode) {
		double time = position->frame_time;
		cerr << "Seconds: " << time << " : " << endl;
		/*time /= 60.0;
		cerr << "Minutes: " << time << " : ";
		time /= 60.0;
		cerr << "Hours: " << time << " : ";*/
	} else {
		cerr << "No timecode." << endl;
	}

	
	((OutputPort<sample>*)m_ports.at(0))->buffer(0)->set(spb, 0, 0);
	((OutputPort<sample>*)m_ports.at(1))->buffer(0)->set(bpb, 0, 0);
	
	// fill the trigger buffers with zeros
	((OutputPort<sample>*)m_ports.at(2))->buffer(0)->set(0.0f, 0, nframes - 1);
	((OutputPort<sample>*)m_ports.at(3))->buffer(0)->set(0.0f, 0, nframes - 1);
	
	// if the transport is rolling, add triggers at the right frame positions
	if ((position->valid & JackTransportBBT) && (state == JackTransportRolling)) {
		double frames_per_beat = position->frame_rate * spb;
		double first_beat = (1.0f - position->tick / position->ticks_per_beat) * frames_per_beat;
		int first_beat_no = position->beat;
		if (first_beat >= frames_per_beat) {
			first_beat -= frames_per_beat;
			--first_beat_no;
		}
		for ( ; first_beat < nframes; first_beat += frames_per_beat) {
			((OutputPort<sample>*)m_ports.at(2))->buffer(0)->set(1.0f, size_t(first_beat));
			if (first_beat_no % int(bpb) == 0) {
				((OutputPort<sample>*)m_ports.at(3))->buffer(0)->set(1.0f, size_t(first_beat));
				++first_beat_no;
			}
		}
	}
	#endif
}


} // namespace Om

