/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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
#include <string>
#include <ladspa.h>
#include <raul/Symbol.hpp>
#include "shared/LV2URIMap.hpp"
#include "LADSPAPlugin.hpp"
#include "LADSPANode.hpp"
#include "Engine.hpp"
#include "Driver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


LADSPAPlugin::LADSPAPlugin(
		Shared::LV2URIMap& uris,
		const std::string& library_path,
		const std::string& uri,
		unsigned long      id,
		const std::string& label,
		const std::string& name)
	: PluginImpl(uris, Plugin::LADSPA, uri, library_path)
	, _id(id)
	, _label(label)
	, _name(name)
{
	set_property(uris.rdf_type,   uris.ingen_LADSPAPlugin);
	set_property(uris.lv2_symbol, Symbol::symbolify(label));
	set_property(uris.doap_name,  name);
}


const Raul::Atom&
LADSPAPlugin::get_property(const Raul::URI& uri) const
{
	if (uri == _uris.doap_name)
		return _name;
	else
		return ResourceImpl::get_property(uri);
}


NodeImpl*
LADSPAPlugin::instantiate(BufferFactory&    bufs,
                          const string&     name,
                          bool              polyphonic,
                          Ingen::PatchImpl* parent,
                          Engine&           engine)
{
	assert(_id != 0);

	const SampleCount srate = engine.driver()->sample_rate();

	union {
		void*                      dp;
		LADSPA_Descriptor_Function fp;
	} df;
	df.dp = NULL;
	df.fp = NULL;

	LADSPANode* n = NULL;

	load(); // FIXME: unload at some point
	assert(_module);
	assert(*_module);

	if (!_module->get_symbol("ladspa_descriptor", df.dp)) {
		warn << _library_path << " is not a LADSPA plugin library" << endl;
		return NULL;
	}

	// Attempt to find the plugin in library
	LADSPA_Descriptor* descriptor = NULL;
	for (unsigned long i=0; (descriptor = (LADSPA_Descriptor*)df.fp(i)) != NULL; ++i) {
		if (descriptor->UniqueID == _id) {
			break;
		}
	}

	if (descriptor != NULL) {
		n = new LADSPANode(this, name, polyphonic, parent, descriptor, srate);

		if ( ! n->instantiate(bufs) ) {
			delete n;
			n = NULL;
		}

	} else {
		error << "Could not find plugin `" << _id << "' in " << _library_path << endl;
	}

	return n;
}


} // namespace Ingen
