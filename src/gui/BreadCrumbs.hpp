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

#ifndef INGEN_GUI_BREADCRUMBS_HPP
#define INGEN_GUI_BREADCRUMBS_HPP

#include "GraphView.hpp"

#include <ingen/Message.hpp>
#include <ingen/URI.hpp>
#include <ingen/client/GraphModel.hpp>
#include <raul/Path.hpp>

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/object.h>
#include <gtkmm/togglebutton.h>
#include <sigc++/signal.h>

#include <cassert>
#include <list>
#include <memory>
#include <string>

namespace ingen::gui {

class App;

/** Collection of breadcrumb buttons forming a path.
 * This doubles as a cache for GraphViews.
 *
 * \ingroup GUI
 */
class BreadCrumbs : public Gtk::HBox
{
public:
	explicit BreadCrumbs(App& app);

	std::shared_ptr<GraphView> view(const raul::Path& path);

	void build(const raul::Path& path, const std::shared_ptr<GraphView>& view);

	sigc::signal<void, const raul::Path&, std::shared_ptr<GraphView>>
	    signal_graph_selected;

private:
	/** Breadcrumb button.
	 *
	 * Each Breadcrumb stores a reference to a GraphView for quick switching.
	 * So, the amount of allocated GraphViews at a given time is equal to the
	 * number of visible breadcrumbs (which is the perfect cache for GUI
	 * responsiveness balanced with mem consumption).
	 *
	 * \ingroup GUI
	 */
	class BreadCrumb : public Gtk::ToggleButton
	{
	public:
		BreadCrumb(const raul::Path&                 path,
		           const std::shared_ptr<GraphView>& view)
		    : _path(path), _view(view)
		{
			assert(!view || view->graph()->path() == path);
			set_border_width(0);
			set_path(path);
			set_can_focus(false);
			show_all();
		}

		explicit BreadCrumb(const raul::Path& path)
			: BreadCrumb{path, nullptr}
		{}

		void set_view(const std::shared_ptr<GraphView>& view) {
			assert(!view || view->graph()->path() == _path);
			_view = view;
		}

		const raul::Path&          path() const { return _path; }
		std::shared_ptr<GraphView> view() const { return _view; }

		void set_path(const raul::Path& path) {
			remove();
			const char* text = (path.is_root()) ? "/" : path.symbol();
			Gtk::Label* lab = manage(new Gtk::Label(text));
			lab->set_padding(0, 0);
			lab->show();
			add(*lab);

			if (_view && _view->graph()->path() != path) {
				_view.reset();
			}
		}

	private:
		raul::Path                 _path;
		std::shared_ptr<GraphView> _view;
	};

	BreadCrumb* create_crumb(const raul::Path&                 path,
	                         const std::shared_ptr<GraphView>& view = nullptr);

	void breadcrumb_clicked(BreadCrumb* crumb);

	void message(const Message& msg);
	void object_destroyed(const URI& uri);
	void object_moved(const raul::Path& old_path, const raul::Path& new_path);

	raul::Path             _active_path;
	raul::Path             _full_path;
	bool                   _enable_signal{true};
	std::list<BreadCrumb*> _breadcrumbs;
};

} // namespace ingen::gui

#endif // INGEN_GUI_BREADCRUMBS_HPP
