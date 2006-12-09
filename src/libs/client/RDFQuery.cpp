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

#include <iostream>
#include <cassert>
#include <rasqal.h>
#include "RDFQuery.h"

namespace Ingen {
namespace Client {


RDFQuery::Results
RDFQuery::run(const Glib::ustring filename)
{
	Results result;

	rasqal_init();

	rasqal_query *rq = rasqal_new_query("sparql", NULL);

	rasqal_query_prepare(rq, (unsigned char*)_query.c_str(), NULL);
	rasqal_query_results* results = rasqal_query_execute(rq);
	assert(results);

	while (!rasqal_query_results_finished(results)) {
		
		Bindings bindings;
		
		for (int i=0; i < rasqal_query_results_get_bindings_count(results); i++) {
			const unsigned char* rname  = rasqal_query_results_get_binding_name(results, i);
			rasqal_literal*      rvalue = rasqal_query_results_get_binding_value(results, i);

			Glib::ustring name((const char*)rname);
			Glib::ustring value((const char*)rasqal_literal_as_string(rvalue));
		
			bindings.insert(std::make_pair(name, value));
		}

		result.push_back(bindings);
		rasqal_query_results_next(results);
	}

	rasqal_free_query_results(results);
	rasqal_free_query(rq);
	rasqal_finish();

	return result;
}


} // namespace Client
} // namespace Ingen
