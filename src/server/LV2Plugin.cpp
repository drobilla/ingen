/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <string>
#include <glibmm.h>

#include "ingen/shared/URIs.hpp"
#include "raul/log.hpp"
#include "sord/sordmm.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"
#include "NodeImpl.hpp"

using namespace std;

namespace Ingen {
namespace Server {

LV2Plugin::LV2Plugin(SharedPtr<LV2Info> lv2_info, const std::string& uri)
	: PluginImpl(*lv2_info->world().uris().get(), Plugin::LV2, uri)
	, _lilv_plugin(NULL)
	, _lv2_info(lv2_info)
{
	set_property(_uris.rdf_type, _uris.lv2_Plugin);
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
			return Raul::Path::nameify(symbol);
		else
			working = working.substr(0, last_slash);
	}

	return "lv2_symbol";
}

NodeImpl*
LV2Plugin::instantiate(BufferFactory& bufs,
                       const string&  name,
                       bool           polyphonic,
                       PatchImpl*     parent,
                       Engine&        engine)
{
	const SampleCount srate = engine.driver()->sample_rate();

	load(); // FIXME: unload at some point

	LV2Node* n = new LV2Node(this, name, polyphonic, parent, srate);

	if ( ! n->instantiate(bufs) ) {
		delete n;
		n = NULL;
	}

	return n;
}

void
LV2Plugin::lilv_plugin(const LilvPlugin* p)
{
	_lilv_plugin = p;
}

const std::string&
LV2Plugin::library_path() const
{
	static const std::string empty_string;
	if (_library_path.empty()) {
		const LilvNode* n = lilv_plugin_get_library_uri(_lilv_plugin);
		if (n) {
			_library_path = lilv_uri_to_path(lilv_node_as_uri(n));
		} else {
			Raul::warn << uri() << " has no library path" << std::endl;
			return empty_string;
		}
	}

	return _library_path;
}

} // namespace Server
} // namespace Ingen
