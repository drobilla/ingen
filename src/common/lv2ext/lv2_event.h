/* lv2_events.h - C header file for the LV2 events extension.
 * 
 * Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
 * Copyright (C) 2008 Dave Robillard <dave@drobilla.net>
 * 
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
 */

#ifndef LV2_EVENTS_H
#define LV2_EVENTS_H

#include <stdint.h>

/** @file
 * This header defines the code portion of the LV2 events extension with URI
 * <http://lv2plug.in/ns/ext/events> (preferred prefix 'lv2ev').
 *
 * This extension is a generic transport mechanism for time stamped events
 * of any type (e.g. MIDI, OSC, ramps, etc).  Each port can transport mixed
 * events of any type; the type of events is defined by a URI (in some other
 * extension) which is mapped to an integer by the host for performance
 * reasons.
 */


/** The PPQN (pulses per quarter note) of tempo-based LV2 event time stamps.
 * Equal to (2^5 * 3^4 * 5 * 7 * 11 * 13 * 17 * 19), which is evenly divisuble
 * by all integers from 1 through 22 inclusive. */
static const uint32_t LV2_EVENT_PPQN = 4190266080;



/** Opaque host data passed to LV2_Event_Feature.
 */
typedef void* LV2_Event_Callback_Data;



/** The data part of the LV2_Feature for the LV2 events extension.
 *
 * The host MUST pass an LV2_Feature struct to the plugin's instantiate
 * method with URI set to "http://lv2plug.in/ns/ext/events" and data
 * set to an instance of this struct.
 *
 * The plugin MUST pass callback_data to any invocation of the functions
 * defined in this struct.  Otherwise, callback_data is opaque and the
 * plugin MUST NOT interpret its value in any way.
 */
typedef struct {

	/** Get the numeric version of an event type from the host.
	 *
	 * The returned value is 0 if there is no type by that URI.
	 * The returned value correlates to the type field of LV2_Event, and is
	 * guaranteed never to change over the course of the plugin's
	 * instantiation, unless the returned value is 0 (i.e. new event types
	 * can be added, but existing ones can never be changed/removed).
	 */
	uint16_t (*uri_to_event_type)(LV2_Event_Callback_Data callback_data,
	                              char const*             uri);
	
	LV2_Event_Callback_Data callback_data;

} LV2_Event_Feature;



/** An LV2 event.
 *
 * LV2 events are generic time-stamped containers for any type of event.
 * The type field defines the format of a given event's contents.
 *
 * This struct defined the header of an LV2 event.  An LV2 event is a single
 * chunk of POD (plain old data), usually contained in a flat buffer
 * (see LV2_EventBuffer below).  Unless a required feature says otherwise,
 * hosts may assume a deep copy of an LV2 event can be created safely
 * using a simple:
 *
 * memcpy(ev_copy, ev, sizeof(LV2_Event) + ev->size);  (or equivalent)
 */
typedef struct {

	/** The frames portion of timestamp.  The units used here can optionally be
	 * set for a port (with the lv2ev:timeUnits property), otherwise this
	 * is audio frames, corresponding to the sample_count parameter of the
	 * LV2 run method (e.g. frame 0 is the first frame for that call to run).
	 */
	uint32_t frames;

	/** The sub-frames portion of timestamp.  The units used here can
	 * optionally be set for a port (with the lv2ev:timeUnits property),
	 * otherwise this is 1/(2^32) of an audio frame.
	 */
	uint32_t subframes;
	
	/** The type of this event, where this numeric value refers to some URI
	 * defining an event type.  The LV2_Event_Feature passed from the host
	 * provides a function to map URIs to event types for this field.
	 * The type 0 is a special nil value, meaning the event has no type and
	 * should be ignored or passed through without interpretation.
	 * Plugins MUST gracefully ignore or pass through any events of a type
	 * which the plugin does not recognize.
	 */
	uint16_t type;

	/** The size of the data portion of this event in bytes, which immediately
	 * follows.  The header size (12 bytes) is not included in this value.
	 */
	uint16_t size;

	/* size bytes of data follow here */

} LV2_Event;



/** A buffer of LV2 events.
 *
 * Like events (which this contains) an event buffer is a single chunk of POD:
 * the entire buffer (including contents) can be copied with a single memcpy.
 * The first contained event begins sizeof(LV2_EventBuffer) bytes after
 * the start of this struct.
 *
 * After this header, the buffer contains an event header (defined by struct
 * LV2_Event), followed by that event's contents (padded to 64 bits), followed by
 * another header, etc:
 *
 * |       |       |       |       |       |       |
 * | | | | | | | | | | | | | | | | | | | | | | | | |
 * |FRAMES |SUBFRMS|TYP|LEN|DATA..DATA..PAD|FRAMES | ...
 */
typedef struct {

	/** The number of events in this buffer.
	 * INPUT PORTS: The host must set this field to the number of events
	 *     contained in the data buffer before calling run().
	 *     The plugin must not change this field.
	 * OUTPUT PORTS: The plugin must set this field to the number of events it
	 *     has written to the buffer before returning from run().
	 *     Any initial value should be ignored by the plugin.
	 */
	uint32_t event_count;

	/** The size of the data buffer in bytes.
	 * This is set by the host and must not be changed by the plugin.
	 * The host is allowed to change this between run() calls.
	 */
	uint32_t capacity;

	/** The size of the initial portion of the data buffer containing data.
	 *  INPUT PORTS: The host must set this field to the number of bytes used
	 *      by all events it has written to the buffer (including headers)
	 *      before calling the plugin's run().
	 *      The plugin must not change this field.
	 *  OUTPUT PORTS: The plugin must set this field to the number of bytes
	 *      used by all events it has written to the buffer (including headers)
	 *      before returning from run().
	 *      Any initial value should be ignored by the plugin.
	 */
	uint32_t size;

} LV2_Event_Buffer;


#endif // LV2_EVENTS_H

