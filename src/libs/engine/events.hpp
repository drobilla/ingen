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

#include "../../../../config/config.h"

#include "DeactivateEvent.hpp"
#include "EnablePatchEvent.hpp"
#include "DisablePatchEvent.hpp"
#include "ClearPatchEvent.hpp"
#include "SetPortValueEvent.hpp"
#include "SetPortValueQueuedEvent.hpp"
#include "ConnectionEvent.hpp"
#include "DisconnectionEvent.hpp"
#include "AddPortEvent.hpp"
#include "AddNodeEvent.hpp"
#include "CreatePatchEvent.hpp"
#include "DestroyEvent.hpp"
#include "SetMetadataEvent.hpp"
#include "RequestMetadataEvent.hpp"
#include "RequestObjectEvent.hpp"
#include "RequestPluginEvent.hpp"
#include "RequestPortValueEvent.hpp"
#include "RequestAllObjectsEvent.hpp"
#include "RequestPluginsEvent.hpp"
#include "LoadPluginsEvent.hpp"
#include "NoteOnEvent.hpp"
#include "NoteOffEvent.hpp"
#include "AllNotesOffEvent.hpp"
#include "DisconnectNodeEvent.hpp"
#include "RegisterClientEvent.hpp"
#include "UnregisterClientEvent.hpp"
#include "RenameEvent.hpp"
#include "PingQueuedEvent.hpp"
#include "MidiLearnEvent.hpp"

#ifdef HAVE_DSSI
#include "DSSIUpdateEvent.hpp"
#include "DSSIControlEvent.hpp"
#include "DSSIConfigureEvent.hpp"
#include "DSSIProgramEvent.hpp"
#endif

#endif // EVENTS_H

