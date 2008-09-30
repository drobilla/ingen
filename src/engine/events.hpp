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

#ifndef EVENTS_H
#define EVENTS_H

#include CONFIG_H_PATH

#include "events/AllNotesOffEvent.hpp"
#include "events/ClearPatchEvent.hpp"
#include "events/ConnectionEvent.hpp"
#include "events/CreateNodeEvent.hpp"
#include "events/CreatePatchEvent.hpp"
#include "events/CreatePortEvent.hpp"
#include "events/DeactivateEvent.hpp"
#include "events/DestroyEvent.hpp"
#include "events/DisconnectAllEvent.hpp"
#include "events/DisconnectionEvent.hpp"
#include "events/EnablePatchEvent.hpp"
#include "events/LoadPluginsEvent.hpp"
#include "events/MidiLearnEvent.hpp"
#include "events/NoteEvent.hpp"
#include "events/PingQueuedEvent.hpp"
#include "events/RegisterClientEvent.hpp"
#include "events/RenameEvent.hpp"
#include "events/RequestAllObjectsEvent.hpp"
#include "events/RequestMetadataEvent.hpp"
#include "events/RequestObjectEvent.hpp"
#include "events/RequestPluginEvent.hpp"
#include "events/RequestPluginsEvent.hpp"
#include "events/RequestPortValueEvent.hpp"
#include "events/SetMetadataEvent.hpp"
#include "events/SetPolyphonicEvent.hpp"
#include "events/SetPolyphonyEvent.hpp"
#include "events/SetPortValueEvent.hpp"
#include "events/UnregisterClientEvent.hpp"

#endif // EVENTS_H

