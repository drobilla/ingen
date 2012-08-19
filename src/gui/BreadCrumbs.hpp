/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

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

#include <list>

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>

#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"

#include "ingen/client/GraphModel.hpp"

#include "GraphView.hpp"

namespace Ingen {
namespace GUI {

/** Collection of breadcrumb buttons forming a path.
 * This doubles as a cache for GraphViews.
 *
 * \ingroup GUI
 */
class BreadCrumbs : public Gtk::HBox
{
public:
	explicit BreadCrumbs(App& app);

	SharedPtr<GraphView> view(const Raul::Path& path);

	void build(Raul::Path path, SharedPtr<GraphView> view);

	sigc::signal<void, const Raul::Path&, SharedPtr<GraphView> > signal_graph_selected;

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
		BreadCrumb(const Raul::Path& path, SharedPtr<GraphView> view = SharedPtr<GraphView>())
			: _path(path)
			, _view(view)
		{
			assert(!view || view->graph()->path() == path);
			set_border_width(0);
			set_path(path);
			set_can_focus(false);
			show_all();
		}

		void set_view(SharedPtr<GraphView> view) {
			assert(!view || view->graph()->path() == _path);
			_view = view;
		}

		const Raul::Path&    path() const { return _path; }
		SharedPtr<GraphView> view() const { return _view; }

		void set_path(const Raul::Path& path) {
			remove();
			const char* text = (path.is_root()) ? "/" : path.symbol();
			Gtk::Label* lab = manage(new Gtk::Label(text));
			lab->set_padding(0, 0);
			lab->show();
			add(*lab);

			if (_view && _view->graph()->path() != path)
				_view.reset();
		}

	private:
		Raul::Path           _path;
		SharedPtr<GraphView> _view;
	};

	BreadCrumb* create_crumb(const Raul::Path&    path,
                             SharedPtr<GraphView> view = SharedPtr<GraphView>());

	void breadcrumb_clicked(BreadCrumb* crumb);

	void object_destroyed(const Raul::URI& uri);
	void object_moved(const Raul::Path& old_path, const Raul::Path& new_path);

	Raul::Path             _active_path;
	Raul::Path             _full_path;
	bool                   _enable_signal;
	std::list<BreadCrumb*> _breadcrumbs;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_BREADCRUMBS_HPP
