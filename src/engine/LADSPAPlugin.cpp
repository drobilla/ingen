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
#include <ladspa.h>
#include <iostream>
#include <raul/Symbol.hpp>
#include "LADSPAPlugin.hpp"
#include "LADSPANode.hpp"
#include "Engine.hpp"
#include "Driver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


LADSPAPlugin::LADSPAPlugin(
		const std::string& library_path,
		const std::string& uri,
		unsigned long      id,
		const std::string& label,
		const std::string& name)
	: PluginImpl(Plugin::LADSPA, uri, library_path)
	, _id(id)
	, _label(label)
	, _name(Raul::Atom::STRING, name)
{
	set_property("rdf:type",   Atom(Atom::URI,    "ingen:LADSPAPlugin"));
	set_property("lv2:symbol", Atom(Atom::STRING, Symbol::symbolify(label)));
	set_property("doap:name",  Atom(Atom::STRING, name));
}


const Raul::Atom&
LADSPAPlugin::get_property(const Raul::URI& uri) const
{
	if (uri.str() == "doap:name")
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

	SampleCount srate       = engine.driver()->sample_rate();
	SampleCount buffer_size = engine.driver()->buffer_size();

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
		cerr << "Looks like this isn't a LADSPA plugin." << endl;
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
		n = new LADSPANode(this, name, polyphonic, parent, descriptor, srate, buffer_size);

		if ( ! n->instantiate(bufs) ) {
			delete n;
			n = NULL;
		}

	} else {
		cerr << "Could not find plugin \"" << _id << "\" in " << _library_path << endl;
	}

	return n;
}


} // namespace Ingen
