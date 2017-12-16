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

#include <string>

#include <boost/variant.hpp>

#include "ingen/Resource.hpp"
#include "ingen/Status.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "raul/Path.hpp"

namespace Raul {
class Atom;
class Path;
class URI;
}

namespace Ingen {

struct BundleBegin
{
};

struct BundleEnd
{
};

struct Connect
{
	Raul::Path tail;
	Raul::Path head;
};

struct Copy
{
	Raul::URI old_uri;
	Raul::URI new_uri;
};

struct Del
{
	Raul::URI uri;
};

struct Delta
{
	Raul::URI       uri;
	Properties      remove;
	Properties      add;
	Resource::Graph ctx;
};

struct Disconnect
{
	Raul::Path tail;
	Raul::Path head;
};

struct DisconnectAll
{
	Raul::Path graph;
	Raul::Path path;
};

struct Error
{
	std::string message;
};

struct Get
{
	Raul::URI subject;
};

struct Move
{
	Raul::Path old_path;
	Raul::Path new_path;
};

struct Put
{
	Raul::URI       uri;
	Properties      properties;
	Resource::Graph ctx;
};

struct Redo
{
};

struct Response
{
	int32_t     id;
	Status      status;
	std::string subject;
};

struct SetProperty
{
	Raul::URI       subject;
	Raul::URI       predicate;
	Atom            value;
	Resource::Graph ctx;
};

struct Undo
{
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

}  // namespace Ingen

#endif  // INGEN_MESSAGE_HPP
