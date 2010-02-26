/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_EVENTS_HPP
#define INGEN_ENGINE_EVENTS_HPP

#include "ingen-config.h"

#include "events/AllNotesOff.hpp"
#include "events/Connect.hpp"
#include "events/CreateNode.hpp"
#include "events/CreatePatch.hpp"
#include "events/CreatePort.hpp"
#include "events/Deactivate.hpp"
#include "events/Delete.hpp"
#include "events/Disconnect.hpp"
#include "events/DisconnectAll.hpp"
#include "events/Get.hpp"
#include "events/LoadPlugins.hpp"
#include "events/Move.hpp"
#include "events/Note.hpp"
#include "events/Ping.hpp"
#include "events/RegisterClient.hpp"
#include "events/RequestMetadata.hpp"
#include "events/RequestPlugins.hpp"
#include "events/SetMetadata.hpp"
#include "events/SetPortValue.hpp"
#include "events/UnregisterClient.hpp"

#endif // INGEN_ENGINE_EVENTS_HPP

