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

#include "Loader.h"
#include <fstream>
#include <cassert>
#include <string>
#include "PatchLibrarian.h"
#include "PatchModel.h"
#include "PatchController.h"
using std::cout; using std::endl;

namespace Ingenuity {


// LoadPatchEvent //

void
LoadPatchEvent::execute()
{
	assert(m_patch_librarian != NULL);
	m_patch_librarian->load_patch(m_patch_model, m_wait, m_merge);
}



// SavePatchEvent //

void
SavePatchEvent::execute()
{
	assert(m_patch_librarian != NULL);
	m_patch_librarian->save_patch(m_patch_model, m_filename, m_recursive);
}

/*
void
LoadSessionEvent::execute()
{
	std::ifstream is;
	is.open(m_filename.c_str(), std::ios::in);

	if ( ! is.good()) {
		cout << "[Loader] Unable to open session file " << m_filename << endl;
		return;
	} else {
		cout << "[Loader] Loading session from " << m_filename << endl;
	}
	string s;

	is >> s;
	if (s != "version") {
		cout << "[Loader] Corrupt session file." << endl;
		is.close();
		return;
	}

	is >> s;
	if (s != "1") {
		cout << "[Loader] Unrecognised session file version." << endl;
		is.close();
		return;
	}
		
	while (!is.eof()) {
		is >> s;
		if (s == "") continue;

		if (s != "patch") {
			//cerr << "[Loader] Corrupt session file, aborting session load." << endl;
			break;
		} else {
			is >> s;
			PatchModel* pm = new PatchModel("", 0);
			if (s.substr(0, 1) != "/")
				s = m_filename.substr(0, m_filename.find_last_of("/")+1) + s;
			pm->filename(s);
			pm->parent(NULL);
			m_patch_librarian->load_patch(pm);
		}
	}

	is.close();
}


void
SaveSessionEvent::execute()
{
	assert(m_filename != "");
	string dir = m_filename.substr(0, m_filename.find_last_of("/"));
	
	string patch_filename;
	
	std::ofstream os;
	os.open(m_filename.c_str(), std::ios::out);
	
	if ( ! os.good()) {
		cout << "[Loader] Unable to write to session file " << m_filename << endl;
		return;
	} else {
		cout << "[Loader] Saving session to " << m_filename << endl;
	}
	
	os << "version 1" << endl;
	
	for (map<string,PatchController*>::iterator i = app->patches().begin();
			i != app->patches().end(); ++i)
	{
		if ((*i).second->model()->parent() == NULL) {
			patch_filename = (*i).second->model()->filename();
			
			// Make path relative if possible
			if (patch_filename.length() > dir.length() &&
					patch_filename.substr(0, dir.length()) == dir)
				patch_filename = patch_filename.substr(dir.length()+1);
			
			os << "patch " << patch_filename << endl;
		}
	}

	os.close();
}
*/


//////// Loader //////////


Loader::Loader(CountedPtr<ModelEngineInterface> engine)
: m_patch_librarian(new PatchLibrarian(engine)),
  m_event(NULL),
  m_thread_exit_flag(false)
{
	assert(m_patch_librarian != NULL);
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_cond, NULL);

	// FIXME: rework this so the thread is only present when it's doing something (save mem)
	launch();
}


Loader::~Loader()
{
	m_thread_exit_flag = true;
	pthread_join(m_thread, NULL);
	delete m_patch_librarian;
}


void
Loader::set_event(LoaderEvent* ev)
{
	assert(ev != NULL);

	pthread_mutex_lock(&m_mutex);
	assert(m_event == NULL);
	m_event = ev;
	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutex);
}


void
Loader::launch()
{
	pthread_create(&m_thread, NULL, Loader::thread_function, this);
}


void*
Loader::thread_function(void* me)
{
	Loader* ct = static_cast<Loader*>(me);
	return ct->m_thread_function(NULL);
}


void*
Loader::m_thread_function(void *)
{
	while ( ! m_thread_exit_flag) {
		pthread_mutex_lock(&m_mutex);
		pthread_cond_wait(&m_cond, &m_mutex);
		
		LoaderEvent* ev = m_event;
		ev->execute();
		delete ev;
		m_event = NULL;
		
		pthread_mutex_unlock(&m_mutex);
	}

	pthread_exit(NULL);
	return NULL;
}


void
Loader::load_patch(PatchModel* model, bool wait, bool merge)
{
	set_event(new LoadPatchEvent(m_patch_librarian, model, wait, merge));
}


void
Loader::save_patch(PatchModel* model, const string& filename, bool recursive)
{
	cout << "[Loader] Saving patch " << filename << endl;
	set_event(new SavePatchEvent(m_patch_librarian, model, filename, recursive));
}


/*
void
Loader::load_session(const string& filename)
{
	set_event(new LoadSessionEvent(m_patch_librarian, filename));
}


void
Loader::save_session(const string& filename)
{
	cout << "Saving session..." << endl;
	set_event(new SaveSessionEvent(m_patch_librarian, filename));
}
*/

} // namespace Ingenuity
