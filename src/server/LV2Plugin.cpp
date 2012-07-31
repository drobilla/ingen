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

#include <string>

#include "ingen/URIs.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "LV2Node.hpp"
#include "LV2Plugin.hpp"

using namespace std;

namespace Ingen {
namespace Server {

LV2Plugin::LV2Plugin(SharedPtr<LV2Info> lv2_info, const std::string& uri)
	: PluginImpl(lv2_info->world().uris(), Plugin::LV2, uri)
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
	LV2Node* n = new LV2Node(
		this, name, polyphonic, parent, engine.driver()->sample_rate());

	if (!n->instantiate(bufs)) {
		delete n;
		return NULL;
	} else {
		return n;
	}
}

void
LV2Plugin::lilv_plugin(const LilvPlugin* p)
{
	_lilv_plugin = p;
}

} // namespace Server
} // namespace Ingen
