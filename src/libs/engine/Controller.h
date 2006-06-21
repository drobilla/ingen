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

#ifndef CONTROLLER_H
#define CONTROLLER_H

namespace Om {


/** Public interface to Om engine shared library.
 *
 * This is an interface to all the audio-processing related functionality
 * only.  OSC communication, LASH session management, etc, are not part of 
 * this library.
 */
class Controller {
public:
	void init();
	void quit();
	
	void load_plugins();
	
	void activate_engine();
	void deactivate_engine();
	
	void create_patch(const char* path, unsigned int polyphony);
	void clear_patch(const char* path);
	void enable_patch(const char* path);
	void disable_patch(const char* path);
	void rename(const char* old_path, const char* new_path);
	
	void create_node(const char*  path,
	                 const char*  type,
					 const char*  library_name,
					 const char*  library_label,
					 unsigned int polyphony);
	
	void destroy(const char* path);
	
	void connect(const char* src_port_path, const char* dst_port_path);
	void disconnect(const char* src_port_path, const char* dst_port_path);
	void disconnect_all(const char* path);
	
	void set_port_value(const char* path, float value);
	void set_port_value_voice(const char* path, unsigned int voice);
	void set_port_value_slow(const char* path, float value);
	
	void note_on(const char*   node_path,
	             unsigned char note_num,
	       	     unsigned char velocity);
	
	void note_off(const char*   node_path,
	             unsigned char  note_num);
	
	void all_notes_off(const char* node_path);
	void midi_learn(const char* node_path);
	
	void get_metadata();
	void set_metadata();
	void responder_plugins();
	void responder_all_objects();
	void responder_port_value();
};


} // namespace Om

#endif // CONTROLLER_H

