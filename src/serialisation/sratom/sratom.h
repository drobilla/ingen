/*
  Copyright 2012 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file sratom.h API for Sratom, an LV2 Atom RDF serialisation library.
*/

#ifndef SRATOM_SRATOM_H
#define SRATOM_SRATOM_H

#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "serd/serd.h"
#include "sord/sord.h"

#ifdef SRATOM_SHARED
#    ifdef _WIN32
#        define SRATOM_LIB_IMPORT __declspec(dllimport)
#        define SRATOM_LIB_EXPORT __declspec(dllexport)
#    else
#        define SRATOM_LIB_IMPORT __attribute__((visibility("default")))
#        define SRATOM_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef SRATOM_INTERNAL
#        define SRATOM_API SRATOM_LIB_EXPORT
#    else
#        define SRATOM_API SRATOM_LIB_IMPORT
#    endif
#else
#    define SRATOM_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
   @defgroup sratom Sratom
   An LV2 Atom RDF serialisation library.
   @{
*/

/**
   Atom serialiser.
*/
typedef struct SratomImpl Sratom;

/**
   Create a new Atom serialiser.
*/
SRATOM_API
Sratom*
sratom_new(LV2_URID_Map* map);

/**
   Free an Atom serialisation.
*/
SRATOM_API
void
sratom_free(Sratom* sratom);

/**
   Set the sink(s) where sratom will write its output.

   This must be called before calling sratom_write().  If @p pretty_numbers is
   true, numbers will be written as pretty Turtle literals, rather than string
   literals with precise types.  The cost of this is the types might get
   fudged on a round-trip to RDF and back.
*/
SRATOM_API
void
sratom_set_sink(Sratom*           sratom,
                const char*       base_uri,
                SerdStatementSink sink,
                SerdEndSink       end_sink,
                void*             handle,
                bool              pretty_numbers);

/**
   Write an Atom to RDF.
   The serialised atom is written to the sink set by sratom_set_sink().
   @return 0 on success, or a non-zero error code otherwise.
*/
SRATOM_API
int
sratom_write(Sratom*         sratom,
             LV2_URID_Unmap* unmap,
             uint32_t        flags,
             const SerdNode* subject,
             const SerdNode* predicate,
             uint32_t        type,
             uint32_t        size,
             const void*     body);

/**
   Read an Atom from RDF.
   The resulting atom will be written to @p forge.
*/
SRATOM_API
void
sratom_read(Sratom*         sratom,
            LV2_Atom_Forge* forge,
            SordWorld*      world,
            SordModel*      model,
            const SordNode* node);

/**
   Serialise an Atom to a Turtle string.
   The returned string must be free()'d by the caller.
*/
SRATOM_API
char*
sratom_to_turtle(Sratom*         sratom,
                 LV2_URID_Unmap* unmap,
                 const char*     base_uri,
                 const SerdNode* subject,
                 const SerdNode* predicate,
                 uint32_t        type,
                 uint32_t        size,
                 const void*     body);

/**
   Read an Atom from a Turtle string.
   The returned atom must be free()'d by the caller.
*/
SRATOM_API
LV2_Atom*
sratom_from_turtle(Sratom*         sratom,
                   const char*     base_uri,
                   const SerdNode* subject,
                   const SerdNode* predicate,
                   const char*     str);

/**
   A convenient resizing sink for LV2_Atom_Forge.
   The handle must point to an initialized SerdChunk.
*/
SRATOM_API
LV2_Atom_Forge_Ref
sratom_forge_sink(LV2_Atom_Forge_Sink_Handle handle,
                  const void*                buf,
                  uint32_t                   size);

/**
   The corresponding deref function for sratom_forge_sink.
*/
SRATOM_API
LV2_Atom*
sratom_forge_deref(LV2_Atom_Forge_Sink_Handle handle,
                   LV2_Atom_Forge_Ref         ref);

/**
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* SRATOM_SRATOM_H */
