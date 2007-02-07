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

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Ingen { namespace Client { class PortModel; } }
using Ingen::Client::PortModel;
using std::string;

struct Coord { double x; double y; };

namespace Ingenuity {

class Controller;


/** Singleton state manager for the entire app.
 *
 * Stores settings like color preferences, search paths, etc. 
 * (ie any user-defined preferences to be stoed in the rc file).
 *
 * \ingroup Ingenuity
 */
class Configuration
{
public:
	Configuration();
	~Configuration();

	void load_settings(string filename = "");
	void save_settings(string filename = "");

	void apply_settings();

	string patch_path()                   { return _patch_path; }
	void   patch_path(const string& path) { _patch_path = path; }
	
	const string& patch_folder()                    { return _patch_folder; }
	void          set_patch_folder(const string& f) { _patch_folder = f; }
	
	int get_port_color(const PortModel* pi);

private:
	/** Search path for patch files.  Colon delimited, as usual. */
	string _patch_path;

	/** Most recent patch folder shown in open dialog */
	string _patch_folder;
	
	int    _audio_port_color;
	int    _control_port_color;
	int    _midi_port_color;
};


} // namespace Ingenuity

#endif // CONFIG_H

	
