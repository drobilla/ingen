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

#ifndef LOADERTHREAD_H
#define LOADERTHREAD_H

#include <string>
#include <cassert>
using std::string;

namespace LibOmClient {
	class PatchLibrarian;
	class PatchModel;
}
using LibOmClient::PatchLibrarian;
using LibOmClient::PatchModel;


namespace OmGtk {
	
/** Event to run in the Loader thread.
 *
 * \ingroup OmGtk
 */
class LoaderEvent
{
public:
	virtual void execute() = 0;
	virtual ~LoaderEvent() {}
protected:
	LoaderEvent() {}
};


/** Loader thread patch loading event.
 *
 * \ingroup OmGtk
 */
class LoadPatchEvent : public LoaderEvent
{
public:
	LoadPatchEvent(PatchLibrarian* pl, PatchModel* model, bool wait, bool merge)
	: m_patch_librarian(pl), m_patch_model(model), m_wait(wait), m_merge(merge) {}
	virtual ~LoadPatchEvent() {}
	void execute();
private:
	PatchLibrarian* m_patch_librarian;
	PatchModel*     m_patch_model;
	bool            m_wait;
	bool            m_merge;
};


/** Loader thread patch loading event.
 *
 * \ingroup OmGtk
 */
class SavePatchEvent : public LoaderEvent
{
public:
	SavePatchEvent(PatchLibrarian* pl, PatchModel* pm, const string& filename, bool recursive)
	: m_patch_librarian(pl), m_patch_model(pm), m_filename(filename), m_recursive(recursive) {}
	virtual ~SavePatchEvent() {}
	void execute();
private:
	PatchLibrarian* m_patch_librarian;
	PatchModel*     m_patch_model;
	string          m_filename;
	bool            m_recursive;
};

/*
class LoadSessionEvent : public LoaderEvent
{
public:
	LoadSessionEvent(PatchLibrarian* pl, const string& filename)
	: m_patch_librarian(pl), m_filename(filename) {}
	virtual ~LoadSessionEvent() {}
	void execute();
private:
	PatchLibrarian* m_patch_librarian;
	string          m_filename;
};


class SaveSessionEvent : public LoaderEvent
{
public:
	SaveSessionEvent(PatchLibrarian* pl, const string& filename)
	: m_patch_librarian(pl), m_filename(filename) {}
	virtual ~SaveSessionEvent() {}
	void execute();
private:
	PatchLibrarian* m_patch_librarian;
	string          m_filename;
};
*/

/** Thread for loading patch files.
 *
 * This is a seperate thread so it can send all the loading message without
 * blocking everything else, so the app can respond to the incoming events
 * caused as a result of the patch loading.
 *
 * \ingroup OmGtk
 */
class Loader
{
public:
	Loader(PatchLibrarian* const patch_librarian);
	~Loader() { m_thread_exit_flag = true; }

	void launch();
	void exit() { m_thread_exit_flag = true; }
	
	void load_patch(PatchModel* model, bool wait, bool merge);
	void save_patch(PatchModel* model, const string& filename, bool recursive);
	
	//void load_session(const string& filename);
	//void save_session(const string& filename);
	
	static void* thread_function(void* me);

private:	
	void* m_thread_function(void*);
	
	void set_event(LoaderEvent* ev);

	PatchLibrarian* const m_patch_librarian;
	LoaderEvent*          m_event;
	bool                  m_thread_exit_flag;
	pthread_t             m_thread;
	pthread_mutex_t       m_mutex;
	pthread_cond_t        m_cond;

};


} // namespace OmGtk

#endif // LOADERRTHREAD_H
