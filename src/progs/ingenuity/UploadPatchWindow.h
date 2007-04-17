/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef UPLOADPATCHWINDOW_H
#define UPLOADPATCHWINDOW_H

#include <sstream>
#include <libglademm/xml.h>
#include <gtkmm.h>
#include <curl/curl.h>
#include "raul/SharedPtr.h"
#include "raul/Thread.h"
#include "raul/AtomicInt.h"
#include "PatchModel.h"
#include "PluginModel.h"
using Ingen::Client::PatchModel;
using Ingen::Client::MetadataMap;

namespace Ingenuity {

class UploadPatchWindow;


class UploadThread : public Raul::Thread {
public:
	UploadThread(UploadPatchWindow* win,
			const string& str,
			const string& url);

private:
	static size_t curl_read_cb(void* ptr, size_t size, size_t nmemb, void *stream);
	static int curl_progress_cb(void* thread, double dltotal, double dlnow, double ultotal, double ulnow);
	
	void _run();

	CURL*              _curl;
	curl_slist*        _headers;
	UploadPatchWindow* _win;
	size_t             _length;
	std::istringstream _stream;
	std::string        _url;
};


/* Upload patch dialog.
 *
 * \ingroup Ingenuity
 */
class UploadPatchWindow : public Gtk::Dialog
{
public:
	UploadPatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);

	void present(SharedPtr<PatchModel> patch);

	Gtk::ProgressBar* progress_bar() { return _upload_progress; }

	void set_response(int response) { _response = response; }
	void set_progress(int pct)      { _progress_pct = pct; }
	
private:
	bool is_symbol(const Glib::ustring& str);
	void symbol_changed();
	void short_name_changed();
	void cancel_clicked();
	void upload_clicked();
	void on_show();
	void on_hide();
	bool progress_callback();

	UploadThread* _thread;

	SharedPtr<PatchModel> _patch;
	
	Raul::AtomicInt _progress_pct;
	Raul::AtomicInt _response;

	Gtk::Entry*       _symbol_entry;
	Gtk::Entry*       _short_name_entry;
	Gtk::ProgressBar* _upload_progress;
	Gtk::Button*      _cancel_button;
	Gtk::Button*      _upload_button;

};
 

} // namespace Ingenuity

#endif // UPLOADPATCHWINDOW_H
