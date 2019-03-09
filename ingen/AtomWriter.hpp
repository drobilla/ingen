/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ATOMWRITER_HPP
#define INGEN_ATOMWRITER_HPP

#include "ingen/AtomForge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Message.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URI.hpp"
#include "ingen/ingen.h"
#include "lv2/atom/forge.h"
#include "lv2/urid/urid.h"

#include <cstdint>

namespace Raul { class Path; }

namespace ingen {

class AtomSink;
class URIMap;
class URIs;

/** An Interface that writes LV2 atoms to an AtomSink. */
class INGEN_API AtomWriter : public Interface
{
public:
	using result_type = void; ///< For boost::apply_visitor

	AtomWriter(serd::World& world, URIMap& map, URIs& uris, AtomSink& sink);

	URI uri() const override { return URI("ingen:/clients/atom_writer"); }

	void message(const Message& message) override;

	void operator()(const BundleBegin&);
	void operator()(const BundleEnd&);
	void operator()(const Connect&);
	void operator()(const Copy&);
	void operator()(const Del&);
	void operator()(const Delta&);
	void operator()(const Disconnect&);
	void operator()(const DisconnectAll&);
	void operator()(const Error&);
	void operator()(const Get&);
	void operator()(const Move&);
	void operator()(const Put&);
	void operator()(const Redo&);
	void operator()(const Response&);
	void operator()(const SetProperty&);
	void operator()(const Undo&);

private:
	void forge_uri(const URI& uri);
	void forge_properties(const Properties& properties);
	void forge_arc(const Raul::Path& tail, const Raul::Path& head);
	void forge_request(LV2_Atom_Forge_Frame* frame, LV2_URID type, int32_t id);
	void forge_context(Resource::Graph ctx);

	void finish_msg();

	URIMap&   _map;
	URIs&     _uris;
	AtomSink& _sink;
	AtomForge _forge;
};

} // namespace ingen

#endif // INGEN_ATOMWRITER_HPP
