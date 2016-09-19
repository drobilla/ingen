/*
  This file is part of Ingen.
  Copyright 2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_UNDOSTACK_HPP
#define INGEN_ENGINE_UNDOSTACK_HPP

#include <ctime>
#include <deque>

#include "ingen/AtomSink.hpp"
#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "serd/serd.h"
#include "sratom/sratom.h"

namespace Ingen {

class URIMap;
class URIs;

namespace Server {

class INGEN_API UndoStack : public AtomSink {
public:
	struct Entry {
		Entry(time_t time=0) : time(time) {}

		Entry(const Entry& copy)
			: time(copy.time)
		{
			for (const LV2_Atom* ev : copy.events) {
				push_event(ev);
			}
		}

		~Entry() { clear(); }

		Entry& operator=(const Entry& rhs) {
			clear();
			time = rhs.time;
			for (const LV2_Atom* ev : rhs.events) {
				push_event(ev);
			}
			return *this;
		}

		void clear() {
			for (LV2_Atom* ev : events) {
				free(ev);
			}
			events.clear();
		}

		void push_event(const LV2_Atom* ev) {
			const uint32_t size = lv2_atom_total_size(ev);
			LV2_Atom*      copy = (LV2_Atom*)malloc(size);
			memcpy(copy, ev, size);
			events.push_front(copy);
		}

		time_t                time;
		std::deque<LV2_Atom*> events;
	};

	UndoStack(URIs& uris, URIMap& map) : _uris(uris), _map(map), _depth(0) {}

	int  start_entry();
	bool write(const LV2_Atom* msg);
	int  finish_entry();

	bool  empty() const { return _stack.empty(); }
	Entry pop();

	void save(FILE* stream, const char* name="undo");

private:
	bool ignore_later_event(const LV2_Atom* first,
	                        const LV2_Atom* second) const;

	void write_entry(Sratom*         sratom,
	                 SerdWriter*     writer,
	                 const SerdNode* subject,
	                 const Entry&    entry);

	URIs&             _uris;
	URIMap&           _map;
	std::deque<Entry> _stack;
	int               _depth;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_UNDOSTACK_HPP
