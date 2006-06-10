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

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace LibOmClient { class PortModel; }
using LibOmClient::PortModel;
using std::string;

struct Coord { double x; double y; };

namespace OmGtk {

class Controller;


/** Singleton state manager for the entire app.
 *
 * Stores settings like color preferences, search paths, etc. 
 * (ie any user-defined preferences to be stoed in the rc file).
 *
 * \ingroup OmGtk
 */
class Configuration
{
public:
	Configuration();
	~Configuration();

	void load_settings(string filename = "");
	void save_settings(string filename = "");

	void apply_settings();

	string patch_path()                   { return m_patch_path; }
	void   patch_path(const string& path) { m_patch_path = path; }
	
	const string& patch_folder()                    { return m_patch_folder; }
	void          set_patch_folder(const string& f) { m_patch_folder = f; }
	
	int get_port_color(const PortModel* pi);

private:
	/** Search path for patch files.  Colon delimited, as usual. */
	string m_patch_path;

	/** Most recent patch folder shown in open dialog */
	string m_patch_folder;
	
	int    m_audio_port_color;
	int    m_control_port_color;
	int    m_midi_port_color;
};


} // namespace OmGtk

#endif // CONFIG_H

	
