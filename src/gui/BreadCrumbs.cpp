/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

// cppcheck-suppress-file constVariablePointer

#include "BreadCrumbs.hpp"

#include "App.hpp"
#include "GraphView.hpp"

#include <ingen/Message.hpp>
#include <ingen/URI.hpp>
#include <ingen/client/GraphModel.hpp>
#include <ingen/client/SigClientInterface.hpp>
#include <raul/Path.hpp>
#include <raul/Symbol.hpp>

#include <sigc++/adaptors/bind.h>
#include <sigc++/functors/mem_fun.h>

#include <algorithm>
#include <string>
#include <variant>

namespace ingen::gui {

using std::string;

BreadCrumbs::BreadCrumbs(App& app)
	: _active_path("/")
	, _full_path("/")
{
	app.sig_client()->signal_message().connect(
		sigc::mem_fun(this, &BreadCrumbs::message));

	set_can_focus(false);
}

std::shared_ptr<GraphView>
BreadCrumbs::view(const raul::Path& path)
{
	const auto b = std::find_if(_breadcrumbs.begin(),
	                            _breadcrumbs.end(),
	                            [&path](const auto* crumb) {
		                            return crumb->path() == path;
	                            });

	return b == _breadcrumbs.end() ? nullptr : (*b)->view();
}

/** Sets up the crumbs to display `path`.
 *
 * If `path` is already part of the shown path, it will be selected and the
 * children preserved.
 */
void
BreadCrumbs::build(const raul::Path&                 path,
                   const std::shared_ptr<GraphView>& view)
{
	const bool old_enable_signal = _enable_signal;
	_enable_signal = false;

	if (!_breadcrumbs.empty() && (path.is_parent_of(_full_path) || path == _full_path)) {
		// Moving to a path we already contain, just switch the active button
		for (auto* b : _breadcrumbs) {
			if (b->path() == path) {
				b->set_active(true);
				if (!b->view()) {
					b->set_view(view);
				}

				// Views are expensive, having two around is a bug
				assert(b->view() == view);

			} else {
				b->set_active(false);
			}
		}

		_active_path = path;

	} else if (!_breadcrumbs.empty() && path.is_child_of(_full_path)) {
		// Moving to a child of the full path, append crumbs (preserve cache)

		string suffix = path.substr(_full_path.length());
		while (!suffix.empty()) {
			if (suffix[0] == '/') {
				suffix = suffix.substr(1);
			}
			const string name = suffix.substr(0, suffix.find('/'));
			_full_path = _full_path.child(raul::Symbol(name));
			BreadCrumb* but = create_crumb(_full_path, view);
			pack_start(*but, false, false, 1);
			_breadcrumbs.push_back(but);
			but->show();
			if (suffix.find('/') == string::npos) {
				break;
			}

			suffix = suffix.substr(suffix.find('/') + 1);
		}

		for (auto* b : _breadcrumbs) {
			b->set_active(false);
		}
		_breadcrumbs.back()->set_active(true);

	} else {
		/* Rebuild from scratch.  Getting here is bad unless absolutely
		   necessary, since the GraphView cache is lost. */

		_full_path = path;
		_active_path = path;

		// Empty existing breadcrumbs
		for (auto* b : _breadcrumbs) {
			remove(*b);
		}
		_breadcrumbs.clear();

		// Add root
		BreadCrumb* root_but = create_crumb(raul::Path("/"), view);
		pack_start(*root_but, false, false, 1);
		_breadcrumbs.push_front(root_but);
		root_but->set_active(root_but->path() == _active_path);

		raul::Path working_path("/");
		string suffix = path.substr(1);
		while (!suffix.empty()) {
			if (suffix[0] == '/') {
				suffix = suffix.substr(1);
			}
			const string name = suffix.substr(0, suffix.find('/'));
			working_path = working_path.child(raul::Symbol(name));
			BreadCrumb* but = create_crumb(working_path, view);
			pack_start(*but, false, false, 1);
			_breadcrumbs.push_back(but);
			but->set_active(working_path == _active_path);
			but->show();
			if (suffix.find('/') == string::npos) {
				break;
			}

			suffix = suffix.substr(suffix.find('/')+1);
		}
	}

	_enable_signal = old_enable_signal;
}

/** Create a new crumb, assigning it a reference to `view` if their paths
 * match, otherwise ignoring `view`.
 */
BreadCrumbs::BreadCrumb*
BreadCrumbs::create_crumb(const raul::Path&                 path,
                          const std::shared_ptr<GraphView>& view)
{
	BreadCrumb* but = manage(new BreadCrumb(
	    path, ((view && path == view->graph()->path()) ? view : nullptr)));

	but->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(this, &BreadCrumbs::breadcrumb_clicked),
		           but));

	return but;
}

void
BreadCrumbs::breadcrumb_clicked(BreadCrumb* crumb)
{
	if (_enable_signal) {
		_enable_signal = false;

		if (!crumb->get_active()) {
			// Tried to turn off the current active button, bad user, no cookie
			crumb->set_active(true);
		} else {
			signal_graph_selected.emit(crumb->path(), crumb->view());
			if (crumb->path() != _active_path) {
				crumb->set_active(false);
			}
		}
		_enable_signal = true;
	}
}

void
BreadCrumbs::message(const Message& msg)
{
	if (const Del* const del = std::get_if<Del>(&msg)) {
		object_destroyed(del->uri);
	}
}

void
BreadCrumbs::object_destroyed(const URI& uri)
{
	const auto i = std::find_if(_breadcrumbs.begin(),
	                            _breadcrumbs.end(),
	                            [&uri](const auto& b) {
		                            return b->path() == uri.c_str();
	                            });

	if (i != _breadcrumbs.end()) {
		// Remove all crumbs after the removed one (inclusive)
		for (auto j = i; j != _breadcrumbs.end();) {
			BreadCrumb* const bc = *j;

			j = _breadcrumbs.erase(j);
			remove(*bc);
		}
	}
}

void
BreadCrumbs::object_moved(const raul::Path& old_path, const raul::Path& new_path)
{
	for (auto* b : _breadcrumbs) {
		if (b->path() == old_path) {
			b->set_path(new_path);
		}
	}
}

} // namespace ingen::gui
