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


#ifndef PLUGINLIBRARY_H
#define PLUGINLIBRARY_H

#include <iostream>
#include <string>
#include <boost/utility.hpp>
#include <dlfcn.h>
using std::string;
using std::cerr; using std::endl;


namespace Ingen {


/** Representation of a shared library containing at least one Plugin.
 *
 * In the NodeFactory, this represents one loaded shared library instance,
 * which is what handle() returns.
 */
class PluginLibrary : boost::noncopyable
{
public:
	/** Construct a new PluginLibrary.
	 *
	 * Path is assumed to be the path of a valid shared library that can be
	 * successfully dlopen'ed.
	 */
	PluginLibrary(const string& path)
	: m_path(path), m_handle(NULL)
	{}
	
	~PluginLibrary()
	{
		close();
	}
	
	/** Load and resolve all symbols in this shared library
	 * (dlopen with RTLD_NOW).
	 *
	 * It is okay to call this many times, the library will only be opened
	 * once.
	 */
	void open()
	{
		if (m_handle == NULL) {
			dlerror();
			m_handle = dlopen(m_path.c_str(), RTLD_NOW);
			if (m_handle == NULL)
				cerr << "[PluginLibrary] Warning:  Error opening shared library "
					<< m_path << "(" << dlerror() << ")" << endl;
		}
	}
	
	/** Close the dynamic library.
	 *
	 * This can be called on an already closed PluginLibrary without problems.
	 */
	void close()
	{
		if (m_handle != NULL) {
			dlerror();
			if (dlclose(m_handle))
				cerr << "[PluginLibrary] Error closing shared library " << m_path
					<< "(" << dlerror() << ")" << endl;
		}
		m_handle = NULL;
	}

	void* handle() const { return m_handle; }

private:
	string m_path;
	void*  m_handle;
};


} // namespace Ingen

#endif // PLUGINLIBRARY_H

