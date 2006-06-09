/* This file is part of Om.  Copyright (C) 2005 Dave Robillard.
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

#include <string>
#include <sstream>
#include <iostream>
#include <lo/lo.h>
#include "OSCModelEngineInterface.h"
#include "ConsoleClientHooks.h"


OSCModelEngineInterface* comm;
ConsoleClientHooks* client_hooks;

int
main()
{
	std::string input;
	std::string s;
	std::string path;
	std::string types;
	lo_message m;
	
	client_hooks = new ConsoleClientHooks();
	comm = new OSCModelEngineInterface(client_hooks);
	comm->attach("1234");

	while (input != "q") {
		std::cout << "> ";
		std::getline(std::cin, input);
		std::istringstream stream(input);

			if (std::cin.eof()) break;

		if (input == "") continue;
		
		//std::cout << "INPUT = " << input << std::endl;
	
		m = lo_message_new();
		stream >> path;
		
		char cs[256];

		stream >> types;

		for (uint i=0; stream >> s && i < types.length(); ++i) {
			// Take care of quoted strings
			if (s[0] == '\"') {
				s = s.substr(1); // hack off begin quote

				if (s[s.length()-1] == '\"') {  // single quoted word
					s = s.substr(0, s.length()-1);
				} else {
					std::string temp;
					do {
						stream >> temp;
						if (temp.find('\n') != std::string::npos) {
							std::cerr << "Mismatched quotes.\n" << std::endl;
							break;
						}
						s += ' ';
						s += temp;
					} while (temp[temp.length()-1] != '\"');
					s = s.substr(0, s.length() - 1); // hack off end quote
				}
			}

			switch (types[i]) {
			case 'f':
				//std::cerr << "float" << std::endl;
				lo_message_add_float(m, atof(s.c_str()));
				break;
			case 's':
				strncpy(cs, s.c_str(), 256);
				//std::cerr << "string" << std::endl;
				lo_message_add_string(m, cs);
				break;
			case 'i':
				//std::cerr << "integer" << std::endl;
				lo_message_add_int32(m, atoi(s.c_str()));
				break;
			default:
				std::cerr << "Unknown type \'" << types[i] << "\'" << std::endl;
				exit(1);
			}
		}
		
		//std::cout << "Sending message..." << std::endl;
		//lo_message_pp(m);
		//usleep(500);
		lo_send_message(comm->addr(), path.c_str(), m);
		lo_message_free(m);
		//std::cout << "Input = " << input << std::endl;
		input = "";
	}
	return 0;
}
