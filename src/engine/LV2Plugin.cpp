/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <cassert>
#include <glibmm.h>
#include "redlandmm/World.hpp"
#include "LV2Plugin.hpp"
#include "LV2Node.hpp"
#include "NodeImpl.hpp"
#include "Engine.hpp"
#include "AudioDriver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


LV2Plugin::LV2Plugin(SharedPtr<LV2Info> lv2_info, const std::string& uri)
	: PluginImpl(Plugin::LV2, uri)
	, _slv2_plugin(NULL)
	, _lv2_info(lv2_info)
{
	set_property("rdf:type", Atom(Atom::URI, "lv2:Plugin"));
}

const string
LV2Plugin::symbol() const
{
	string working = uri().str();
	if (working[working.length()-1] == '/')
		working = working.substr(0, working.length()-1);

	while (working.length() > 0) {
		size_t last_slash = working.find_last_of("/");
		const string symbol = working.substr(last_slash+1);
		if ( (symbol[0] >= 'a' && symbol[0] <= 'z')
				|| (symbol[0] >= 'A' && symbol[0] <= 'Z') )
			return Path::nameify(symbol);
		else
			working = working.substr(0, last_slash);
	}

	return "lv2_symbol";
}


NodeImpl*
LV2Plugin::instantiate(BufferFactory&    bufs,
                       const string&     name,
                       bool              polyphonic,
                       Ingen::PatchImpl* parent,
                       Engine&           engine)
{
	SampleCount srate       = engine.audio_driver()->sample_rate();
	SampleCount buffer_size = engine.audio_driver()->buffer_size();

	load(); // FIXME: unload at some point

	Glib::Mutex::Lock lock(engine.world()->rdf_world->mutex());
	LV2Node* n = new LV2Node(this, name, polyphonic, parent, srate, buffer_size);

	if ( ! n->instantiate(bufs) ) {
		delete n;
		n = NULL;
	}

	return n;
}


void
LV2Plugin::slv2_plugin(SLV2Plugin p)
{
	_slv2_plugin = p;
}


const std::string&
LV2Plugin::library_path() const
{
	static const std::string empty_string;
	if (_library_path == "") {
		SLV2Value v = slv2_plugin_get_library_uri(_slv2_plugin);
		if (v) {
			_library_path = slv2_uri_to_path(slv2_value_as_uri(v));
		} else {
			cerr << "WARNING: Plugin " << uri() << " has no library path" << endl;
			return empty_string;
		}
	}

	return _library_path;
}


} // namespace Ingen
