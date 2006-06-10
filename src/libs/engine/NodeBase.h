/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NODEBASE_H
#define NODEBASE_H

#include <string>
#include <cstdlib>
#include "Node.h"
using std::string;

namespace Om {

class Plugin;
class Patch;
namespace Shared {
	class ClientInterface;
} using Shared::ClientInterface;


/** Common implementation stuff for Node.
 *
 * Pretty much just attributes and getters/setters are here.
 *
 * \ingroup engine
 */
class NodeBase : public Node
{
public:
	NodeBase(const string& name, size_t poly, Patch* parent, samplerate srate, size_t buffer_size);

	virtual ~NodeBase();

	virtual void activate();
	virtual void deactivate();
	bool activated() { return m_activated; }

	virtual void run(size_t nframes);
		
	virtual void set_port_buffer(size_t voice, size_t port_num, void* buf) {}
	
	virtual void add_to_patch() {}
	virtual void remove_from_patch() {}
	
	void add_to_store();
	void remove_from_store();
	
	//void send_creation_messages(ClientInterface* client) const;
	
	size_t num_ports() const { return m_num_ports; }
	size_t poly() const      { return m_poly; }
	bool   traversed() const { return m_traversed; }
	void   traversed(bool b) { m_traversed = b; }
	
	const Array<Port*>& ports() const { return m_ports; }

	virtual List<Node*>* providers()               { return m_providers; }
	virtual void         providers(List<Node*>* l) { m_providers = l; }
	
	virtual List<Node*>* dependants()               { return m_dependants; }
	virtual void         dependants(List<Node*>* l) { m_dependants = l; }
	
	Patch* parent_patch() const { return (m_parent == NULL) ? NULL : m_parent->as_patch(); }

	virtual const Plugin* plugin() const                 { exit(EXIT_FAILURE); }
	virtual void          plugin(const Plugin* const pi) { exit(EXIT_FAILURE); }
	
	void set_path(const Path& new_path);
	
protected:	
	// Disallow copies (undefined)
	NodeBase(const NodeBase&);
	NodeBase& operator=(const NodeBase&);
	
	size_t      m_poly;

	samplerate  m_srate;
	size_t      m_buffer_size;
	bool        m_activated;

	size_t       m_num_ports; // number of ports PER VOICE
	Array<Port*> m_ports;

	bool         m_traversed;
	List<Node*>* m_providers;     // Nodes connected to this one's input ports
	List<Node*>* m_dependants;    // Nodes this one's output ports are connected to
};


} // namespace Om

#endif // NODEBASE_H
