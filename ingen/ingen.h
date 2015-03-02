/*
  This file is part of Ingen.
  Copyright 2014 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_H
#define INGEN_H

#ifdef INGEN_SHARED
#    ifdef _WIN32
#        define INGEN_LIB_IMPORT __declspec(dllimport)
#        define INGEN_LIB_EXPORT __declspec(dllexport)
#    else
#        define INGEN_LIB_IMPORT __attribute__((visibility("default")))
#        define INGEN_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef INGEN_INTERNAL
#        define INGEN_API INGEN_LIB_EXPORT
#    else
#        define INGEN_API INGEN_LIB_IMPORT
#    endif
#else
#    define INGEN_API
#endif

#define INGEN_NS "http://drobilla.net/ns/ingen#"

#define INGEN__Arc            INGEN_NS "Arc"
#define INGEN__Block          INGEN_NS "Block"
#define INGEN__Graph          INGEN_NS "Graph"
#define INGEN__GraphPrototype INGEN_NS "GraphPrototype"
#define INGEN__Internal       INGEN_NS "Internal"
#define INGEN__Node           INGEN_NS "Node"
#define INGEN__Plugin         INGEN_NS "Plugin"
#define INGEN__activity       INGEN_NS "activity"
#define INGEN__arc            INGEN_NS "arc"
#define INGEN__block          INGEN_NS "block"
#define INGEN__broadcast      INGEN_NS "broadcast"
#define INGEN__canvasX        INGEN_NS "canvasX"
#define INGEN__canvasY        INGEN_NS "canvasY"
#define INGEN__enabled        INGEN_NS "enabled"
#define INGEN__file           INGEN_NS "file"
#define INGEN__head           INGEN_NS "head"
#define INGEN__incidentTo     INGEN_NS "incidentTo"
#define INGEN__polyphonic     INGEN_NS "polyphonic"
#define INGEN__polyphony      INGEN_NS "polyphony"
#define INGEN__prototype      INGEN_NS "prototype"
#define INGEN__sprungLayout   INGEN_NS "sprungLayout"
#define INGEN__tail           INGEN_NS "tail"
#define INGEN__uiEmbedded     INGEN_NS "uiEmbedded"
#define INGEN__value          INGEN_NS "value"

#endif // INGEN_H
