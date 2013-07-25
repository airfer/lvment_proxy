/* packet.h
 * Definitions for packet disassembly structures and routines
 *
 * $Id: packet.h,v 1.72 2003/10/01 07:11:45 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __PACKET_H__
#define __PACKET_H__

#include "wiretap/wtap.h"
#include "proto.h"
#include "tvbuff.h"
#include "pint.h"
#include "to_str.h"
#include "value_string.h"
#include "column_info.h"
#include "frame_data.h"
#include "packet_info.h"
#include "column-utils.h"
#include "epan.h"

#define hi_nibble(b) (((b) & 0xf0) >> 4)
#define lo_nibble(b) ((b) & 0x0f)

/* Useful when you have an array whose size you can tell at compile-time */
#define array_length(x)  (sizeof x / sizeof x[0])

/* Check whether the "len" bytes of data starting at "offset" is
 * entirely inside the captured data for this packet. */
#define	BYTES_ARE_IN_FRAME(offset, captured_len, len) \
	((guint)(offset) + (guint)(len) > (guint)(offset) && \
	 (guint)(offset) + (guint)(len) <= (guint)(captured_len))

/* To pass one of two strings, singular or plural */
#define plurality(d,s,p) ((d) == 1 ? (s) : (p))

typedef struct _packet_counts {
  gint           sctp;
  gint           tcp;
  gint           udp;
  gint           icmp;
  gint           ospf;
  gint           gre;
  gint           netbios;
  gint           ipx;
  gint           vines;
  gint           other;
  gint           total;
  gint           arp;
} packet_counts;

/* Types of character encodings */
typedef enum {
	CHAR_ASCII	 = 0,	/* ASCII */
	CHAR_EBCDIC	 = 1	/* EBCDIC */
} char_enc;

/* Struct for boolean enumerations */
typedef struct true_false_string {
	char	*true_string;
	char	*false_string;
} true_false_string;

extern const true_false_string flags_set_truth;

extern void packet_init(void);
extern void packet_cleanup(void);

/* Handle for dissectors you call directly or register with "dissector_add()".
   This handle is opaque outside of "packet.c". */
struct dissector_handle;
typedef struct dissector_handle *dissector_handle_t;

/* Hash table for matching port numbers and dissectors; this is opaque
   outside of "packet.c". */
struct dissector_table;
typedef struct dissector_table *dissector_table_t;

/*
 * Dissector that returns nothing.
 */
typedef void (*dissector_t)(tvbuff_t *, packet_info *, proto_tree *);

/*
 * Dissector that returns:
 *
 *	the amount of data in the protocol's PDU, if it was able to
 *	dissect all the data;
 *
 *	0, if the tvbuff doesn't contain a PDU for that protocol;
 *
 *	the negative of the amount of additional data needed, if
 *	we need more data (e.g., from subsequent TCP segments) to
 *	dissect the entire PDU.
 */
typedef int (*new_dissector_t)(tvbuff_t *, packet_info *, proto_tree *);

typedef void (*DATFunc) (gchar *table_name, ftenum_t selector_type,
    gpointer key, gpointer value, gpointer user_data);
typedef void (*DATFunc_handle) (gchar *table_name, gpointer value,
    gpointer user_data);
typedef void (*DATFunc_table) (gchar *table_name, gchar *ui_name,
    gpointer user_data);

/* Opaque structure - provides type checking but no access to components */
typedef struct dtbl_entry dtbl_entry_t;

extern dissector_handle_t dtbl_entry_get_handle (dtbl_entry_t *dtbl_entry);
extern dissector_handle_t dtbl_entry_get_initial_handle (dtbl_entry_t * entry);
extern void dissector_table_foreach_changed (char *name, DATFunc func,
    gpointer user_data);
extern void dissector_table_foreach (char *name, DATFunc func,
    gpointer user_data);
extern void dissector_all_tables_foreach_changed (DATFunc func,
    gpointer user_data);
extern void dissector_table_foreach_handle(char *name, DATFunc_handle func,
    gpointer user_data);
extern void dissector_all_tables_foreach_table (DATFunc_table func,
    gpointer user_data);

/* a protocol uses the function to register a sub-dissector table */
extern dissector_table_t register_dissector_table(const char *name,
    char *ui_name, ftenum_t type, int base);

/* Find a dissector table by table name. */
extern dissector_table_t find_dissector_table(const char *name);

/* Get the UI name for a sub-dissector table, given its internal name */
extern char *get_dissector_table_ui_name(const char *name);

/* Get the field type for values of the selector for a dissector table,
   given the table's internal name */
extern ftenum_t get_dissector_table_selector_type(const char *name);

/* Get the base to use when displaying values of the selector for a
   sub-dissector table, given the table's internal name */
extern int get_dissector_table_base(const char *name);

/* Add an entry to a uint dissector table. */
extern void dissector_add(const char *abbrev, guint32 pattern,
    dissector_handle_t handle);

/* Delete the entry for a dissector in a uint dissector table
   with a particular pattern. */
extern void dissector_delete(const char *name, guint32 pattern,
    dissector_handle_t handle);

/* Change the entry for a dissector in a uint dissector table
   with a particular pattern to use a new dissector handle. */
extern void dissector_change(const char *abbrev, guint32 pattern,
    dissector_handle_t handle);

/* Reset an entry in a uint dissector table to its initial value. */
extern void dissector_reset(const char *name, guint32 pattern);

/* Look for a given value in a given uint dissector table and, if found,
   call the dissector with the arguments supplied, and return TRUE,
   otherwise return FALSE. */
extern gboolean dissector_try_port(dissector_table_t sub_dissectors,
    guint32 port, tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/* Look for a given value in a given uint dissector table and, if found,
   return the dissector handle for that value. */
extern dissector_handle_t dissector_get_port_handle(
    dissector_table_t sub_dissectors, guint32 port);

/* Add an entry to a string dissector table. */
extern void dissector_add_string(const char *name, gchar *pattern,
    dissector_handle_t handle);

/* Delete the entry for a dissector in a string dissector table
   with a particular pattern. */
extern void dissector_delete_string(const char *name, const gchar *pattern,
	dissector_handle_t handle);

/* Change the entry for a dissector in a string dissector table
   with a particular pattern to use a new dissector handle. */
extern void dissector_change_string(const char *name, gchar *pattern,
    dissector_handle_t handle);

/* Reset an entry in a string sub-dissector table to its initial value. */
extern void dissector_reset_string(const char *name, const gchar *pattern);

/* Look for a given string in a given dissector table and, if found, call
   the dissector with the arguments supplied, and return TRUE, otherwise
   return FALSE. */
extern gboolean dissector_try_string(dissector_table_t sub_dissectors,
    const gchar *string, tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/* Look for a given value in a given string dissector table and, if found,
   return the dissector handle for that value. */
extern dissector_handle_t dissector_get_string_handle(
    dissector_table_t sub_dissectors, const gchar *string);

/* Add a handle to the list of handles that *could* be used with this
   table.  That list is used by code in the UI. */
extern void dissector_add_handle(const char *name, dissector_handle_t handle);

/* List of "heuristic" dissectors (which get handed a packet, look at it,
   and either recognize it as being for their protocol, dissect it, and
   return TRUE, or don't recognize it and return FALSE) to be called
   by another dissector. */
typedef GSList *heur_dissector_list_t;

/* Type of a heuristic dissector */
typedef gboolean (*heur_dissector_t)(tvbuff_t *, packet_info *,
	proto_tree *);

/* A protocol uses this function to register a heuristic dissector list */
extern void register_heur_dissector_list(const char *name,
    heur_dissector_list_t *list);

/* Add a sub-dissector to a heuristic dissector list.  Called by the
   protocol routine that wants to register a sub-dissector.  */
extern void heur_dissector_add(const char *name, heur_dissector_t dissector,
    int proto);

/* Try all the dissectors in a given heuristic dissector list until
   we find one that recognizes the protocol, in which case we return
   TRUE, or we run out of dissectors, in which case we return FALSE. */
extern gboolean dissector_try_heuristic(heur_dissector_list_t sub_dissectors,
    tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);

/* Register a dissector. */
extern void register_dissector(const char *name, dissector_t dissector,
    int proto);
extern void new_register_dissector(const char *name, new_dissector_t dissector,
    int proto);

/* Get the short name of the protocol for a dissector handle. */
extern char *dissector_handle_get_short_name(dissector_handle_t handle);

/* Get the index of the protocol for a dissector handle. */
extern int dissector_handle_get_protocol_index(dissector_handle_t handle);

/* Find a dissector by name. */
extern dissector_handle_t find_dissector(const char *name);

/* Create an anonymous handle for a dissector. */
extern dissector_handle_t create_dissector_handle(dissector_t dissector,
    int proto);
extern dissector_handle_t new_create_dissector_handle(new_dissector_t dissector,
    int proto);

/* Call a dissector through a handle. */
extern int call_dissector(dissector_handle_t handle, tvbuff_t *tvb,
    packet_info *pinfo, proto_tree *tree);

/* Do all one-time initialization. */
extern void dissect_init(void);

extern void dissect_cleanup(void);

/*
 * Given a tvbuff, and a length from a packet header, adjust the length
 * of the tvbuff to reflect the specified length.
 */
extern void set_actual_length(tvbuff_t *tvb, guint specified_len);

/* Allow protocols to register "init" routines, which are called before
   we make a pass through a capture file and dissect all its packets
   (e.g., when we read in a new capture file, or run a "filter packets"
   or "colorize packets" pass over the current capture file). */
extern void register_init_routine(void (*func)(void));

/* Initialize all data structures used for dissection. */
extern void init_dissection(void);

/* Allow protocols to register a "cleanup" routine to be
 * run after the initial sequential run through the packets.
 * Note that the file can still be open after this; this is not
 * the final cleanup. */
extern void register_postseq_cleanup_routine(void (*func)(void));

/* Call all the registered "postseq_cleanup" routines. */
extern void postseq_cleanup_all_protocols(void);

/* Allow dissectors to register a "final_registration" routine
 * that is run like the proto_register_XXX() routine, but the end
 * end of the epan_init() function; that is, *after* all other
 * subsystems, liked dfilters, have finished initializing. This is
 * useful for dissector registration routines which need to compile
 * display filters. dfilters can't initialize itself until all protocols
 * have registereed themselvs. */
extern void
register_final_registration_routine(void (*func)(void));

/* Call all the registered "final_registration" routines. */
extern void
final_registration_all_protocols(void);

/*
 * Add a new data source to the list of data sources for a frame, given
 * the tvbuff for the data source and its name.
 */
extern void add_new_data_source(packet_info *pinfo, tvbuff_t *tvb,
    char *name);

/*
 * Free up a frame's list of data sources.
 */
extern void free_data_sources(packet_info *pinfo);

/*
 * Dissectors should never modify the packet data.
 */
extern void dissect_packet(epan_dissect_t *edt,
    union wtap_pseudo_header *pseudo_header, const guchar *pd,
    frame_data *fd, column_info *cinfo);

/* These functions are in packet-ethertype.c */
extern void capture_ethertype(guint16 etype, const guchar *pd, int offset,
		int len, packet_counts *ld);
extern void ethertype(guint16 etype, tvbuff_t *tvb, int offset_after_ethertype,
		packet_info *pinfo, proto_tree *tree, proto_tree *fh_tree,
		int etype_id, int trailer_id, int fcs_len);

#endif /* packet.h */