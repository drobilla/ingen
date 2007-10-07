/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "TransportNode.hpp"
#include <jack/transport.h>
#include "OutputPort.hpp"
#include "PluginImpl.hpp"
#include "JackAudioDriver.hpp"
#include "PortImpl.hpp"
#include "util.hpp"
//#include "Engine.hpp"

namespace Ingen {


TransportNode::TransportNode(const string& path, bool polyphonic, Patch* parent, SampleRate srate, size_t buffer_size)
: NodeBase(new PluginImpl(Plugin::Internal, "ingen:transport_node"), path, false, parent, srate, buffer_size)
{
#if 0
	_num_ports = 10;
	_ports.alloc(_num_ports);

	OutputPort<Sample>* spb_port = new OutputPort<Sample>(this, "Seconds per Beat", 0, 1,
	//	new PortInfo("Seconds per Beat", CONTROL, OUTPUT, 0, 0, 1), 1);
	_ports.at(0) = spb_port;

	OutputPort<Sample>* bpb_port = new OutputPort<Sample>(this, "Beats per Bar", 1, 1,
	//	new PortInfo("Beats per Bar", CONTROL, OUTPUT, 0, 0, 1), 1);
	_ports.at(1) = bpb_port;
	
	OutputPort<Sample>* bar_port = new OutputPort<Sample>(this, "Bar", 3, 1,
//		new PortInfo("Bar", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(2) = bar_port;
	
	OutputPort<Sample>* beat_port = new OutputPort<Sample>(this, "Beat", 3, 1,
	//	new PortInfo("Beat", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(3) = beat_port;
	
	OutputPort<Sample>* frame_port = new OutputPort<Sample>(this, "Frame", 3, 1,
	//	new PortInfo("Frame", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(4) = frame_port;

	OutputPort<Sample>* hour_port = new OutputPort<Sample>(this, "Hour", 3, 1,
	//	new PortInfo("Hour", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(5) = hour_port;
	
	OutputPort<Sample>* minute_port = new OutputPort<Sample>(this, "Minute", 3, 1,
	//	new PortInfo("Minute", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(6) = minute_port;

	OutputPort<Sample>* second_port = new OutputPort<Sample>(this, "Second", 3, 1,
	//	new PortInfo("Second", CONTROL, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(7) = second_port;
	
	OutputPort<Sample>* trg_port = new OutputPort<Sample>(this, "Beat Tick", 2, 1,
	//	new PortInfo("Beat Tick", AUDIO, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(8) = trg_port;

	OutputPort<Sample>* bar_trig_port = new OutputPort<Sample>(this, "Bar Tick", 3, 1,
	//	new PortInfo("Bar Tick", AUDIO, OUTPUT, 0, 0, 1), buffer_size);
	_ports.at(9) = bar_trig_port;
#endif
	PluginImpl* p = const_cast<PluginImpl*>(_plugin);
	p->plug_label("transport");
	assert(p->uri() == "ingen:transport_node");
	p->name("Ingen Transport Node (BROKEN)");
}


void
TransportNode::process(ProcessContext& context)
{
	NodeBase::pre_process(context);
#if 0

	// FIXME: this will die horribly with any driver other than jack (in theory)
	const jack_position_t* const position = ((JackAudioDriver*)Engine::instance().audio_driver())->position();
	jack_transport_state_t state = ((JackAudioDriver*)Engine::instance().audio_driver())->transport_state();
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

	
	((OutputPort<Sample>*)_ports.at(0))->buffer(0)->set(spb, 0, 0);
	((OutputPort<Sample>*)_ports.at(1))->buffer(0)->set(bpb, 0, 0);
	
	// fill the trigger buffers with zeros
	((OutputPort<Sample>*)_ports.at(2))->buffer(0)->set(0.0f, 0, nframes - 1);
	((OutputPort<Sample>*)_ports.at(3))->buffer(0)->set(0.0f, 0, nframes - 1);
	
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
			((OutputPort<Sample>*)_ports.at(2))->buffer(0)->set(1.0f, size_t(first_beat));
			if (first_beat_no % int(bpb) == 0) {
				((OutputPort<Sample>*)_ports.at(3))->buffer(0)->set(1.0f, size_t(first_beat));
				++first_beat_no;
			}
		}
	}
	#endif
	
	NodeBase::post_process(context);
}


} // namespace Ingen

