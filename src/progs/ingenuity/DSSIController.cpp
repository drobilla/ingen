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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "DSSIController.h"
#include <iomanip>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "NodeModel.h"
#include "DSSIModule.h"
#include "Controller.h"

namespace OmGtk {


DSSIController::DSSIController(CountedPtr<NodeModel> model)
: NodeController(model),
  m_banks_dirty(true)
{
	assert(model->plugin()->type() == PluginModel::DSSI);
	Gtk::Menu::MenuList& items = m_menu.items();
	items[0].property_sensitive() = true; // "Show Control Window" item
	
	Gtk::Menu_Helpers::MenuElem program_elem("Select Program", m_program_menu);
	items.push_front(program_elem);
	m_program_menu_item = program_elem.get_child();
	m_program_menu_item->set_sensitive(false);
	
	items.push_front(Gtk::Menu_Helpers::MenuElem("Show Plugin GUI",
		sigc::mem_fun(this, &DSSIController::show_gui)));
}

void
DSSIController::program_add(int bank, int program, const string& name)
{
	node_model()->add_program(bank, program, name);
	m_banks_dirty = true;
}


void
DSSIController::program_remove(int bank, int program)
{
	node_model()->remove_program(bank, program);
	m_banks_dirty = true;
}

/** Trivial wrapper of attempt_to_show_gui for libsigc
 */
void
DSSIController::show_gui()
{
	attempt_to_show_gui();
}


void
DSSIController::update_program_menu()
{
	m_program_menu.items().clear();
	
	const map<int, map<int, string> >& banks = node_model()->get_programs();
	std::ostringstream oss;
	
	map<int, map<int, string> >::const_iterator i;
	for (i = banks.begin(); i != banks.end(); ++i) {
		Gtk::Menu* bank_menu;
		if (banks.size() > 1)
			bank_menu = manage(new Gtk::Menu());
		else
			bank_menu = &m_program_menu;
		map<int, string>::const_iterator j;
		for (j = i->second.begin(); j != i->second.end(); ++j) {
			oss.str("");
			oss << std::setw(3) << std::setfill('0')
				<< j->first << ' ' << j->second;
			sigc::slot<void> slt = bind(
				bind(sigc::mem_fun(*this, &DSSIController::send_program_change),
					j->first), i->first);
			bank_menu->items().push_back(
				Gtk::Menu_Helpers::MenuElem(oss.str(), slt));
		}
		if (banks.size() > 1) {
			oss.str("");
			oss << "Bank " << i->first;
			m_program_menu.items().push_back(
				Gtk::Menu_Helpers::MenuElem(oss.str(), *bank_menu));
		}
	}

	// Disable the program menu if there are no programs
	if (banks.size() == 0)
		m_program_menu_item->set_sensitive(false);
	else
		m_program_menu_item->set_sensitive(true);
	
	m_banks_dirty = false;
}


void
DSSIController::send_program_change(int bank, int program)
{
	Controller::instance().set_program(node_model()->path(), bank, program);
}


void
DSSIController::create_module(OmFlowCanvas* canvas)
{
	//cerr << "Creating DSSI module " << m_model->path() << endl;

	assert(canvas != NULL);
	assert(m_module == NULL);
	
	m_module = new DSSIModule(canvas, this);
	create_all_ports();

	m_module->move_to(node_model()->x(), node_model()->y());
}



/** Attempt to show the DSSI GUI for this plugin.
 *
 * Returns whether or not GUI was successfully loaded/shown.
 */
bool
DSSIController::attempt_to_show_gui()
{
	// Shamelessley "inspired by" jack-dssi-host
	// Copyright 2004 Chris Cannam, Steve Harris and Sean Bolton.

	const bool verbose = false;

	string engine_url = Controller::instance().engine_url();
	// Hack off last character if it's a /
	if (engine_url[engine_url.length()-1] == '/')
		engine_url = engine_url.substr(0, engine_url.length()-1);
	
	const char*   dllName   = node_model()->plugin()->lib_name().c_str();
	const char*   label     = node_model()->plugin()->plug_label().c_str();
	const char*   myName    = "om_gtk";
	const string& oscUrl    = engine_url + "/dssi" + node_model()->path();

	struct dirent* entry = NULL;
	char*          dllBase = strdup(dllName);
	char*          subpath = NULL;
	DIR*           subdir = NULL;
	char*          filename = NULL;
	struct stat    buf;
	int            fuzzy = 0;

	char* env_dssi_path = getenv("DSSI_PATH");
	string dssi_path;
	if (!env_dssi_path) {
	 	cerr << "DSSI_PATH is empty.  Assuming /usr/lib/dssi:/usr/local/lib/dssi." << endl;
		dssi_path = "/usr/lib/dssi:/usr/local/lib/dssi";
	} else {
		dssi_path = env_dssi_path;
	}

	if (strlen(dllBase) > 3 && !strcasecmp(dllBase + strlen(dllBase) - 3, ".so")) {
		dllBase[strlen(dllBase) - 3] = '\0';
	}


	// This is pretty nasty, it loops through the path, even if the dllBase is absolute
	while (dssi_path != "") {
		string directory = dssi_path.substr(0, dssi_path.find(':'));
		if (dssi_path.find(':') != string::npos)
			dssi_path = dssi_path.substr(dssi_path.find(':')+1);
		else
			dssi_path = "";
		
		if (*dllBase == '/') {
			subpath = strdup(dllBase);
		} else {
			subpath = (char*)malloc(strlen(directory.c_str()) + strlen(dllBase) + 2);
			sprintf(subpath, "%s/%s", directory.c_str(), dllBase);
		}
	
		for (fuzzy = 0; fuzzy <= 1; ++fuzzy) {
	
			if (!(subdir = opendir(subpath))) {
				if (verbose) {
					fprintf(stderr, "%s: can't open plugin GUI directory \"%s\"\n", myName, subpath);
				}
				break;
			}
	
			while ((entry = readdir(subdir))) {
	
				if (entry->d_name[0] == '.')
					continue;
				if (!strchr(entry->d_name, '_'))
					continue;
	
				if (fuzzy) {
					if (verbose) {
						fprintf(stderr, "checking %s against %s\n", entry->d_name, dllBase);
					}
					if (strncmp(entry->d_name, dllBase, strlen(dllBase)))
						continue;
				} else {
					if (verbose) {
						fprintf(stderr, "checking %s against %s\n", entry->d_name, label);
					}
					if (strncmp(entry->d_name, label, strlen(label)))
						continue;
				}
	
				filename = (char*)malloc(strlen(subpath) + strlen(entry->d_name) + 2);
				sprintf(filename, "%s/%s", subpath, entry->d_name);
	
				if (stat(filename, &buf)) {
					perror("stat failed");
					free(filename);
					continue;
				}
	
				if ((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) &&
				        (buf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
	
					if (verbose) {
						fprintf(stderr, "%s: trying to execute GUI at \"%s\"\n",
						        myName, filename);
					}
	
					if (fork() == 0) {
						execlp(filename, filename, oscUrl.c_str(), dllName, label,
							node_model()->name().c_str(), 0);
						perror("exec failed");
						exit(1);
					}
	
					free(filename);
					free(subpath);
					free(dllBase);
					return true;
				}
	
				free(filename);
			}
		}
	}
	
	cerr << "Unable to launch DSSI GUI for " << node_model()->path() << endl;
	
	free(subpath);
	free(dllBase);
	return false;
}


void
DSSIController::show_menu(GdkEventButton* event)
{
        if (m_banks_dirty)
	        update_program_menu();
	NodeController::show_menu(event);
}


} // namespace OmGtk

