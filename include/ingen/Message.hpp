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

#ifndef INGEN_MESSAGE_HPP
#define INGEN_MESSAGE_HPP

#include "ingen/Atom.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/URI.hpp"
#include "raul/Path.hpp"

#include <boost/variant/variant.hpp>

#include <cstdint>
#include <string>

namespace ingen {

struct BundleBegin
{
	int32_t seq;
};

struct BundleEnd
{
	int32_t seq;
};

struct Connect
{
	int32_t    seq;
	Raul::Path tail;
	Raul::Path head;
};

struct Copy
{
	int32_t seq;
	URI     old_uri;
	URI     new_uri;
};

struct Del
{
	int32_t seq;
	URI     uri;
};

struct Delta
{
	int32_t         seq;
	URI             uri;
	Properties      remove;
	Properties      add;
	Resource::Graph ctx;
};

struct Disconnect
{
	int32_t    seq;
	Raul::Path tail;
	Raul::Path head;
};

struct DisconnectAll
{
	int32_t    seq;
	Raul::Path graph;
	Raul::Path path;
};

struct Error
{
	int32_t     seq;
	std::string message;
};

struct Get
{
	int32_t seq;
	URI     subject;
};

struct Move
{
	int32_t    seq;
	Raul::Path old_path;
	Raul::Path new_path;
};

struct Put
{
	int32_t         seq;
	URI             uri;
	Properties      properties;
	Resource::Graph ctx;
};

struct Redo
{
	int32_t seq;
};

struct Response
{
	int32_t     id;
	Status      status;
	std::string subject;
};

struct SetProperty
{
	int32_t         seq;
	URI             subject;
	URI             predicate;
	Atom            value;
	Resource::Graph ctx;
};

struct Undo
{
	int32_t seq;
};

using Message = boost::variant<BundleBegin,
                               BundleEnd,
                               Connect,
                               Copy,
                               Del,
                               Delta,
                               Disconnect,
                               DisconnectAll,
                               Error,
                               Get,
                               Move,
                               Put,
                               Redo,
                               Response,
                               SetProperty,
                               Undo>;

}  // namespace ingen

#endif  // INGEN_MESSAGE_HPP
