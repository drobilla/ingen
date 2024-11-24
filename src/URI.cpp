/*
  This file is part of Ingen.
  Copyright 2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ingen/URI.hpp>

#include <ingen/FilePath.hpp>
#include <serd/serd.h>
#include <sord/sordmm.hpp>

#include <cassert>

namespace ingen {

URI::URI() : _uri(SERD_URI_NULL), _node(SERD_NODE_NULL) {}

URI::URI(const std::string& str)
    : _uri(SERD_URI_NULL)
    , _node(serd_node_new_uri_from_string(reinterpret_cast<const uint8_t*>(
                                              str.c_str()),
                                          nullptr,
                                          &_uri))
{
}

URI::URI(const char* str)
    : _uri(SERD_URI_NULL)
    , _node(serd_node_new_uri_from_string(reinterpret_cast<const uint8_t*>(str),
                                          nullptr,
                                          &_uri))
{
}

URI::URI(const std::string& str, const URI& base)
    : _uri(SERD_URI_NULL)
    , _node(serd_node_new_uri_from_string(reinterpret_cast<const uint8_t*>(
                                              str.c_str()),
                                          &base._uri,
                                          &_uri))
{
}

URI::URI(const SerdNode& node)
    : _uri(SERD_URI_NULL)
    , _node(serd_node_new_uri_from_node(&node, nullptr, &_uri))
{
	assert(node.type == SERD_URI);
}

URI::URI(const SerdNode& node, const SerdURI& uri) : _uri(uri), _node(node)
{
	assert(node.type == SERD_URI);
}

URI::URI(const Sord::Node& node) : URI(*node.to_serd_node()) {}

URI::URI(const FilePath& path)
    : _uri(SERD_URI_NULL)
    , _node(
          serd_node_new_file_uri(reinterpret_cast<const uint8_t*>(path.c_str()),
                                 nullptr,
                                 &_uri,
                                 true))
{
}

URI::URI(const URI& uri)
    : _uri(SERD_URI_NULL), _node(serd_node_new_uri(&uri._uri, nullptr, &_uri))
{
}

URI&
URI::operator=(const URI& uri)
{
	if (&uri != this) {
		serd_node_free(&_node);
		_node = serd_node_new_uri(&uri._uri, nullptr, &_uri);
	}

	return *this;
}

URI::URI(URI&& uri) noexcept : _uri(uri._uri), _node(uri._node)
{
	uri._node = SERD_NODE_NULL;
	uri._uri  = SERD_URI_NULL;
}

URI&
URI::operator=(URI&& uri) noexcept
{
	serd_node_free(&_node);

	_node     = uri._node;
	_uri      = uri._uri;
	uri._node = SERD_NODE_NULL;
	uri._uri  = SERD_URI_NULL;
	return *this;
}

URI::~URI()
{
	serd_node_free(&_node);
}

URI
URI::make_relative(const URI& base) const
{
	SerdURI        uri;
	const SerdNode node =
	    serd_node_new_relative_uri(&_uri, &base._uri, nullptr, &uri);

	return {node, uri};
}

URI
URI::make_relative(const URI& base, const URI& root) const
{
	SerdURI        uri;
	const SerdNode node =
	    serd_node_new_relative_uri(&_uri, &base._uri, &root._uri, &uri);

	return {node, uri};
}

} // namespace ingen
