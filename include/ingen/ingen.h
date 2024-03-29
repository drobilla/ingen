/*
  This file is part of Ingen.
  Copyright 2014-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_INGEN_H
#define INGEN_INGEN_H

#if defined(_WIN32) && !defined(INGEN_STATIC) && defined(INGEN_INTERNAL)
#	define INGEN_API __declspec(dllexport)
#elif defined(_WIN32) && !defined(INGEN_STATIC)
#	define INGEN_API __declspec(dllimport)
#elif defined(__GNUC__)
#	define INGEN_API __attribute__((visibility("default")))
#else
#	define INGEN_API
#endif

#define INGEN_NS "http://drobilla.net/ns/ingen#"

#define INGEN__Arc             INGEN_NS "Arc"
#define INGEN__Block           INGEN_NS "Block"
#define INGEN__BundleEnd       INGEN_NS "BundleEnd"
#define INGEN__BundleStart     INGEN_NS "BundleStart"
#define INGEN__Graph           INGEN_NS "Graph"
#define INGEN__GraphPrototype  INGEN_NS "GraphPrototype"
#define INGEN__Internal        INGEN_NS "Internal"
#define INGEN__Node            INGEN_NS "Node"
#define INGEN__Plugin          INGEN_NS "Plugin"
#define INGEN__Redo            INGEN_NS "Redo"
#define INGEN__Undo            INGEN_NS "Undo"
#define INGEN__activity        INGEN_NS "activity"
#define INGEN__arc             INGEN_NS "arc"
#define INGEN__block           INGEN_NS "block"
#define INGEN__broadcast       INGEN_NS "broadcast"
#define INGEN__canvasX         INGEN_NS "canvasX"
#define INGEN__canvasY         INGEN_NS "canvasY"
#define INGEN__enabled         INGEN_NS "enabled"
#define INGEN__externalContext INGEN_NS "externalContext"
#define INGEN__file            INGEN_NS "file"
#define INGEN__head            INGEN_NS "head"
#define INGEN__incidentTo      INGEN_NS "incidentTo"
#define INGEN__internalContext INGEN_NS "internalContext"
#define INGEN__loadedBundle    INGEN_NS "loadedBundle"
#define INGEN__maxRunLoad      INGEN_NS "maxRunLoad"
#define INGEN__meanRunLoad     INGEN_NS "meanRunLoad"
#define INGEN__minRunLoad      INGEN_NS "minRunLoad"
#define INGEN__numThreads      INGEN_NS "numThreads"
#define INGEN__polyphonic      INGEN_NS "polyphonic"
#define INGEN__polyphony       INGEN_NS "polyphony"
#define INGEN__prototype       INGEN_NS "prototype"
#define INGEN__sprungLayout    INGEN_NS "sprungLayout"
#define INGEN__tail            INGEN_NS "tail"
#define INGEN__uiEmbedded      INGEN_NS "uiEmbedded"
#define INGEN__value           INGEN_NS "value"

#endif // INGEN_INGEN_H
