/* $Id$ */
/*
 * Copyright (c) 2003  Noah Levitt <nlevitt@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#ifndef GUCHARMAP_CHARMAP_H
#define GUCHARMAP_CHARMAP_H

#include <gtk/gtk.h>
#include <gucharmap/gucharmap-table.h>


G_BEGIN_DECLS


#define GUCHARMAP_CHARMAP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), gucharmap_charmap_get_type (), \
                                                  GucharmapCharmap))

#define GUCHARMAP_CHARMAP_CLASS(clazz) (G_TYPE_CHECK_CLASS_CAST ((clazz), \
                                                       gucharmap_charmap_get_type (),\
                                                       GucharmapCharmapClass))

#define IS_GUCHARMAP_CHARMAP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                         gucharmap_charmap_get_type ()))

typedef struct _GucharmapCharmap GucharmapCharmap;
typedef struct _GucharmapCharmapClass GucharmapCharmapClass;

typedef enum
{
  GUCHARMAP_CHARMAP_NOT_FOUND,
  GUCHARMAP_CHARMAP_FOUND,
  GUCHARMAP_CHARMAP_WRAPPED,
  GUCHARMAP_CHARMAP_NOTHING_TO_SEARCH_FOR
}
GucharmapCharmapSearchResult;


/* the order of the captions */
typedef enum
{
  GUCHARMAP_CHARMAP_CAPTION_CHARACTER = 0,
  GUCHARMAP_CHARMAP_CAPTION_CATEGORY,
  GUCHARMAP_CHARMAP_CAPTION_DECOMPOSITION,
  GUCHARMAP_CHARMAP_CAPTION_UTF8,
  GUCHARMAP_CHARMAP_CAPTION_OTHER_REPS,

  /* nameslist stuff */
  GUCHARMAP_CHARMAP_CAPTION_EQUALS,
  GUCHARMAP_CHARMAP_CAPTION_STARS,
  GUCHARMAP_CHARMAP_CAPTION_EXES,
  GUCHARMAP_CHARMAP_CAPTION_POUNDS,
  GUCHARMAP_CHARMAP_CAPTION_COLONS,

  /* unihan subset */
  GUCHARMAP_CHARMAP_CAPTION_KDEFINITION,
  GUCHARMAP_CHARMAP_CAPTION_KMANDARIN,
  GUCHARMAP_CHARMAP_CAPTION_KJAPANESEON,
  GUCHARMAP_CHARMAP_CAPTION_KJAPANESEKUN,
  GUCHARMAP_CHARMAP_CAPTION_KCANTONESE,
  GUCHARMAP_CHARMAP_CAPTION_KTANG,
  GUCHARMAP_CHARMAP_CAPTION_KKOREAN,

  GUCHARMAP_CHARMAP_CAPTION_COUNT
}
GucharmapCharmapCaption;

typedef struct 
{
  gunichar start;
  GtkTreePath *tree_path;
} 
BlockIndex;

struct _GucharmapCharmap
{
  GtkVBox parent;

  GucharmapTable *chartable;

  /* rows and columns on a page */
  gint rows, cols;

  /* the unicode block selection list */
  GtkTreeSelection *block_selection;
  GtkTreeStore *block_selector_model;
  GtkWidget *block_selector_view;
  gulong block_selection_changed_handler_id; 

  BlockIndex *block_index;
  gint block_index_size;

  /* the caption */
  GtkTreeRowReference *caption_rows[GUCHARMAP_CHARMAP_CAPTION_COUNT];
  GtkTreeStore *caption_model;
};


struct _GucharmapCharmapClass
{
  GtkVBoxClass parent_class;

  void (* status_message) (GucharmapCharmap *charmap, const gchar *message);
};


GType gucharmap_charmap_get_type (void);
GtkWidget * gucharmap_charmap_new (void);
void gucharmap_charmap_set_font (GucharmapCharmap *charmap, const gchar *font_name);
void gucharmap_charmap_identify_clipboard (GucharmapCharmap *charmap, GtkClipboard *clipboard);
void gucharmap_charmap_expand_block_selector (GucharmapCharmap *charmap);
void gucharmap_charmap_collapse_block_selector (GucharmapCharmap *charmap);
void gucharmap_charmap_go_to_character (GucharmapCharmap *charmap, gunichar uc);
GucharmapCharmapSearchResult gucharmap_charmap_search (GucharmapCharmap *charmap, 
                                    const gchar *search_text, 
                                    gint direction);
void gucharmap_charmap_hide_caption (GucharmapCharmap *charmap, GucharmapCharmapCaption caption_id);
void gucharmap_charmap_show_caption (GucharmapCharmap *charmap, GucharmapCharmapCaption caption_id);
void gucharmap_charmap_zoom_enable (GucharmapCharmap *charmap);
void gucharmap_charmap_zoom_disable (GucharmapCharmap *charmap);
GucharmapTable * gucharmap_charmap_get_chartable (GucharmapCharmap *charmap);

G_END_DECLS

#endif  /* #ifndef GUCHARMAP_CHARMAP_H */
