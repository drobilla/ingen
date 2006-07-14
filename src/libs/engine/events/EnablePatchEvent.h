/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ENABLEPATCHEVENT_H
#define ENABLEPATCHEVENT_H

#include <string>
#include "QueuedEvent.h"

using std::string;

template <typename T> class Array;

namespace Om {

class Patch;
class Node;


/** Enables a patch's DSP processing.
 *
 * \ingroup engine
 */
class EnablePatchEvent : public QueuedEvent
{
public:
	EnablePatchEvent(CountedPtr<Responder> responder, samplecount timestamp, const string& patch_path);
	
	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	string        m_patch_path;
	Patch*        m_patch;
	Array<Node*>* m_process_order; // Patch's new process order
};


} // namespace Om


#endif // ENABLEPATCHEVENT_H
