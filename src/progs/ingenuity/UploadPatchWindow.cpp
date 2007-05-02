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

#include <sstream>
#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <boost/optional/optional.hpp>
#include <curl/curl.h>
#include <raul/RDFQuery.h>
#include "interface/EngineInterface.h"
#include "client/Serializer.h"
#include "client/PatchModel.h"
#include "UploadPatchWindow.h"
#include "App.h"
#include "Configuration.h"
#include "ThreadedLoader.h"

using boost::optional;
using namespace Raul;
using namespace std;

namespace Ingenuity {


UploadPatchWindow::UploadPatchWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Dialog(cobject)
	, _thread(NULL)
	, _progress_pct(0)
	, _response(0)
{
	xml->get_widget("upload_patch_symbol_entry", _symbol_entry);
	xml->get_widget("upload_patch_short_name_entry", _short_name_entry);
	xml->get_widget("upload_patch_progress", _upload_progress);
	xml->get_widget("upload_patch_cancel_button", _cancel_button);
	xml->get_widget("upload_patch_upload_button", _upload_button);
	

	_symbol_entry->signal_changed().connect(sigc::mem_fun(this, &UploadPatchWindow::symbol_changed));
	_short_name_entry->signal_changed().connect(sigc::mem_fun(this, &UploadPatchWindow::short_name_changed));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &UploadPatchWindow::cancel_clicked));
	_upload_button->signal_clicked().connect(sigc::mem_fun(this, &UploadPatchWindow::upload_clicked));
}


void
UploadPatchWindow::present(SharedPtr<PatchModel> patch)
{
	_patch = patch;
	
	Gtk::Window::present();
}


void
UploadPatchWindow::on_show()
{
	Gtk::Dialog::on_show();

	Raul::Atom atom = _patch->get_metadata("lv2:symbol");
	if (atom)
		_symbol_entry->set_text(atom.get_string());
	
	atom = _patch->get_metadata("doap:name");
	if (atom)
		_short_name_entry->set_text(atom.get_string());
}


void
UploadPatchWindow::on_hide()
{
	Gtk::Dialog::on_hide();

	delete _thread;
	_thread = NULL;
}


bool
UploadPatchWindow::is_symbol(const Glib::ustring& s)
{
	if (s.length() == 0)
		return false;

	for (unsigned i=0; i < s.length(); ++i)
		if ( !( (s[i] >= 'a' && s[i] <= 'z')
				|| (s[i] >= 'A' && s[i] <= 'Z')
				|| (s[i] == '_')
				|| (i > 0 && s[i] >= '0' && s[i] <= '9') ) )
			return false;

	return true;
}


void
UploadPatchWindow::symbol_changed()
{
	_upload_button->property_sensitive() = (
			is_symbol(_symbol_entry->get_text())
			&& _short_name_entry->get_text().length() > 0);
}


void
UploadPatchWindow::short_name_changed()
{
	_upload_button->property_sensitive() = (
			is_symbol(_symbol_entry->get_text())
			&& _short_name_entry->get_text().length() > 0);
}

	
size_t
UploadThread::curl_read_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	assert(size == 1);

	istringstream* ss = (istringstream*)data;

	return ss->readsome((char*)ptr, nmemb);
}


int
UploadThread::curl_progress_cb(void   *thread,
                               double dltotal,
                               double dlnow,
                               double ultotal,
                               double ulnow)
{
	UploadThread* me = (UploadThread*)thread;
	me->_win->set_progress(min(
			(int)(min(ulnow, (double)me->_length) / me->_length * 100.0),
			99));
	return 0;
}


UploadThread::UploadThread(UploadPatchWindow* win, const string& str, const string& url)
	: Thread("Upload")
	, _curl(NULL)
	, _headers(NULL)
	, _win(win)
	, _length(str.length())
	, _stream(str)
	, _url(url)
{
	_curl = curl_easy_init();
	_headers = curl_slist_append(NULL, "Content-type: application/x-turtle");

	curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
	curl_easy_setopt(_curl, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(_curl, CURLOPT_READDATA, &_stream);
	curl_easy_setopt(_curl, CURLOPT_READFUNCTION, &UploadThread::curl_read_cb);
	curl_easy_setopt(_curl, CURLOPT_INFILESIZE, sizeof(char) * str.length());
	curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, FALSE);
	curl_easy_setopt(_curl, CURLOPT_PROGRESSFUNCTION, &UploadThread::curl_progress_cb);
	curl_easy_setopt(_curl, CURLOPT_PROGRESSDATA, this);
}


void
UploadThread::_run()
{
	curl_easy_perform(_curl);

	long response;
	curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &response);
	
	printf("Server returned %ld\n", response);

	_win->set_response(response);
	_win->set_progress(100);

	curl_slist_free_all(_headers);
	curl_easy_cleanup(_curl);
	
	_headers = NULL;
	_curl = NULL;
}


bool
UploadPatchWindow::progress_callback()
{
	const int progress = _progress_pct.get();
	const int response = _response.get();

	_upload_progress->set_fraction(progress / 100.0);
	
	if (progress == 100) {
		if (response == 200) {
			_upload_progress->set_text("Transfer completed");
		} else {
			_upload_progress->set_fraction(0.0);
			char status[4];
			snprintf(status, 4, "%d", (unsigned)response);
			string msg = "Transfer failed:  Server returned ";
			msg.append(status);
			_upload_progress->set_text(msg);
		}
		delete _thread;
		_thread = NULL;
		_upload_button->set_sensitive(true);
		return false;
	} else {
		return true;
	}
}


void
UploadPatchWindow::upload_clicked()
{
	assert(!_thread);

	Glib::ustring symbol = _symbol_entry->get_text();
	Glib::ustring short_name = _short_name_entry->get_text();

	_patch->set_metadata("lv2:symbol", Atom(symbol));
	App::instance().engine()->set_metadata(_patch->path(), "lv2:symbol", Atom(symbol));
	
	_patch->set_metadata("doap:name", Atom(short_name));
	App::instance().engine()->set_metadata(_patch->path(), "doap:name", Atom(short_name));

	_response = 0;
	_progress_pct = 0;

	_upload_progress->set_fraction(0.0);
	_upload_progress->set_text("");

	Serializer s(*App::instance().rdf_world());
	s.start_to_string();
	s.serialize(_patch);
	const string str = s.finish();
	istringstream stream(str);

	string url = "http://rdf.drobilla.net/ingen_patches/";
	url += symbol + ".ingen.ttl";

	_thread = new UploadThread(this, str, url);

	_thread->start();
		
	_upload_button->set_sensitive(false);

	Glib::signal_timeout().connect(
		sigc::mem_fun(this, &UploadPatchWindow::progress_callback), 100);
}			


void
UploadPatchWindow::cancel_clicked()
{
	if (_thread) {
		delete _thread;
		_thread = NULL;
	}

	hide();
}


} // namespace Ingenuity
