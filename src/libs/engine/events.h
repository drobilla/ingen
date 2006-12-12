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

#ifndef EVENTS_H
#define EVENTS_H

#include "config.h"

#include "DeactivateEvent.h"
#include "EnablePatchEvent.h"
#include "DisablePatchEvent.h"
#include "ClearPatchEvent.h"
#include "SetPortValueEvent.h"
#include "SetPortValueQueuedEvent.h"
#include "ConnectionEvent.h"
#include "DisconnectionEvent.h"
#include "AddPortEvent.h"
#include "AddNodeEvent.h"
#include "CreatePatchEvent.h"
#include "DestroyEvent.h"
#include "SetMetadataEvent.h"
#include "RequestMetadataEvent.h"
#include "RequestObjectEvent.h"
#include "RequestPluginEvent.h"
#include "RequestPortValueEvent.h"
#include "RequestAllObjectsEvent.h"
#include "RequestPluginsEvent.h"
#include "LoadPluginsEvent.h"
#include "NoteOnEvent.h"
#include "NoteOffEvent.h"
#include "AllNotesOffEvent.h"
#include "DisconnectNodeEvent.h"
#include "RegisterClientEvent.h"
#include "UnregisterClientEvent.h"
#include "RenameEvent.h"
#include "PingQueuedEvent.h"
#include "MidiLearnEvent.h"

#ifdef HAVE_LASH
#include "LashRestoreDoneEvent.h"
#endif

#ifdef HAVE_DSSI
#include "DSSIUpdateEvent.h"
#include "DSSIControlEvent.h"
#include "DSSIConfigureEvent.h"
#include "DSSIProgramEvent.h"
#endif

#endif // EVENTS_H

