/* $Id$ */
/*
 * Copyright (c) 2003 Noah Levitt
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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <gucharmap/gucharmap-mini-fontsel.h>
#include "gucharmap_intl.h"
#include "gucharmap_marshal.h"


enum
{
  MIN_FONT_SIZE = 5,
  MAX_FONT_SIZE = 400,
};

enum
{
  CHANGED,
  NUM_SIGNALS
};

static guint gucharmap_mini_font_selection_signals[NUM_SIGNALS] = { 0 };


/* looks up PangoFontFamily by family name, since no such function is in
 * the api */
static GHashTable *pango_font_family_hash = NULL;


static gint
cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return g_utf8_collate (a_name, b_name);
}


/* also initializes the hash table pango_font_families */
static void
show_available_fonts (GucharmapMiniFontSelection *fontsel)
{
  PangoFontFamily **families;
  GList *family_names = NULL;
  gint n_families, i;

  /* keys are strings */
  pango_font_family_hash = g_hash_table_new (g_str_hash, g_str_equal);
  
  pango_context_list_families (
          gtk_widget_get_pango_context (GTK_WIDGET (fontsel)),
          &families, &n_families);
  qsort (families, n_families, sizeof (PangoFontFamily *), cmp_families);

  for (i = 0;  i < n_families;  i++)
    {
      /* must strdup for the hash */
      gchar *family_name = g_strdup (pango_font_family_get_name (families[i]));

      /* insert into the hash */
      g_hash_table_insert (pango_font_family_hash, family_name, families[i]);

      /* add to the list */
      family_names = g_list_append (family_names, (gpointer) family_name);
    }

  gtk_combo_set_popdown_strings (GTK_COMBO (fontsel->family), family_names);
    
  g_list_free (family_names);
}


static int
compare_font_descriptions (const PangoFontDescription *a, 
                           const PangoFontDescription *b)
{
  int val = strcmp (pango_font_description_get_family (a), 
                    pango_font_description_get_family (b));
  if (val != 0)
    return val;

  if (pango_font_description_get_weight (a) 
      != pango_font_description_get_weight (b))
    return pango_font_description_get_weight (a) 
           - pango_font_description_get_weight (b);

  if (pango_font_description_get_style (a) 
      != pango_font_description_get_style (b))
    return pango_font_description_get_style (a) 
           - pango_font_description_get_style (b);
  
  if (pango_font_description_get_stretch (a) 
      != pango_font_description_get_stretch (b))
    return pango_font_description_get_stretch (a) 
           - pango_font_description_get_stretch (b);

  if (pango_font_description_get_variant (a) 
      != pango_font_description_get_variant (b))
    return pango_font_description_get_variant (a) 
            - pango_font_description_get_variant (b);

  return 0;
}


static int
faces_sort_func (const void *a, const void *b)
{
  PangoFontDescription *desc_a, *desc_b;
  int rv;

  desc_a = pango_font_face_describe (*(PangoFontFace **)a);
  desc_b = pango_font_face_describe (*(PangoFontFace **)b);
  
  rv = compare_font_descriptions (desc_a, desc_b);

  pango_font_description_free (desc_a);
  pango_font_description_free (desc_b);

  return rv;
}


/* This fills the font style list with all the possible style combinations
 * for the current font family. 
 * Also creates fontsel->available_faces. */
static void
show_available_styles (GucharmapMiniFontSelection *fontsel)
{
  PangoFontFace **faces;
  PangoFontFamily *family;
  GList *face_names = NULL;
  gint n_faces, i;

  family = g_hash_table_lookup (
          pango_font_family_hash, 
          pango_font_description_get_family (fontsel->font_desc));
  pango_font_family_list_faces (family, &faces, &n_faces);

  g_return_if_fail (n_faces > 0);

  qsort (faces, n_faces, sizeof (PangoFontFace *), faces_sort_func);

  if (fontsel->available_faces != NULL)
    g_hash_table_destroy (fontsel->available_faces);
  fontsel->available_faces = g_hash_table_new (g_str_hash, g_str_equal);

  for (i = 0;  i < n_faces;  i++)
    {
      const gchar *face_name = pango_font_face_get_face_name (faces[i]);
      face_names = g_list_append (face_names, (gpointer) face_name);

      g_hash_table_insert (fontsel->available_faces, 
                           (gchar *) face_name, faces[i]);
    }

  gtk_combo_set_popdown_strings (GTK_COMBO (fontsel->style), face_names);

  g_list_free (face_names);
  g_free (faces);
}


static void
set_family (GucharmapMiniFontSelection *fontsel, 
            const gchar *new_family)
{
  pango_font_description_set_family (fontsel->font_desc, new_family);

  show_available_styles (fontsel);

  g_signal_emit (fontsel, gucharmap_mini_font_selection_signals[CHANGED], 0);
}


static void 
family_changed (GtkWidget *widget, 
                GucharmapMiniFontSelection *fontsel)
{
  const gchar *new_family;

  new_family = gtk_entry_get_text (
                 GTK_ENTRY (GTK_COMBO (fontsel->family)->entry));

  if (new_family[0] == '\0') /* empty string */
    return;

  set_family (fontsel, new_family);
}


static void
set_style (GucharmapMiniFontSelection *fontsel, 
           const gchar *new_style)
{
  PangoFontFace *face;
  gint size;
  
  face = g_hash_table_lookup (fontsel->available_faces, new_style);
  g_return_if_fail (face != NULL);

  size = pango_font_description_get_size (fontsel->font_desc);
  pango_font_description_free (fontsel->font_desc);

  fontsel->font_desc = pango_font_face_describe (face);
  pango_font_description_set_size (fontsel->font_desc, size);

  g_signal_emit (fontsel, gucharmap_mini_font_selection_signals[CHANGED], 0);
}


static void 
style_changed (GtkWidget *widget, 
               GucharmapMiniFontSelection *fontsel)
{
  const gchar *new_style;

  new_style = gtk_entry_get_text (
          GTK_ENTRY (GTK_COMBO (fontsel->style)->entry));

  if (new_style[0] == '\0') /* empty string */
    return;

  set_style (fontsel, new_style);
}


/* size is in points */
static void
set_size (GucharmapMiniFontSelection *fontsel, 
          gint size)
{
  pango_font_description_set_size (
	  fontsel->font_desc, 
	  PANGO_SCALE * CLAMP (size, MIN_FONT_SIZE, MAX_FONT_SIZE));
  g_signal_emit (fontsel, gucharmap_mini_font_selection_signals[CHANGED], 0);
}


static void 
size_changed (GtkAdjustment *adjustment, 
              GucharmapMiniFontSelection *fontsel)
{
  if ((gint) gtk_adjustment_get_value (adjustment) 
      != pango_font_description_get_size (fontsel->font_desc))
    set_size (fontsel, (gint) gtk_adjustment_get_value (adjustment));
}


static void
realize (GtkWidget *widget)
{
  AtkObject *accessib;
  GucharmapMiniFontSelection *fontsel = GUCHARMAP_MINI_FONT_SELECTION (widget);

  accessib = gtk_widget_get_accessible (GTK_WIDGET (fontsel));
  atk_object_set_name (accessib, _("Font"));

  gtk_box_set_spacing (GTK_BOX (fontsel), 6);

  fontsel->family = gtk_combo_new ();
  gtk_widget_show (fontsel->family);
  accessib = gtk_widget_get_accessible (fontsel->family);
  atk_object_set_name (accessib, _("Font Family"));

  fontsel->style = gtk_combo_new ();
  gtk_widget_show (fontsel->style);
  accessib = gtk_widget_get_accessible (fontsel->style);
  atk_object_set_name (accessib, _("Font Style"));

  fontsel->size = gtk_spin_button_new (GTK_ADJUSTMENT (fontsel->size_adj),
                                       0, 0);
  gtk_widget_show (fontsel->size);
  accessib = gtk_widget_get_accessible (fontsel->size);
  atk_object_set_name (accessib, _("Font Size"));

  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (fontsel->family)->entry),
                             FALSE);
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (fontsel->style)->entry),
                             FALSE);

  show_available_fonts (fontsel);

  g_signal_connect (G_OBJECT (GTK_COMBO (fontsel->family)->entry), "changed",
                    G_CALLBACK (family_changed), fontsel);
  fontsel->style_changed_handler_id = g_signal_connect (
          G_OBJECT (GTK_COMBO (fontsel->style)->entry), "changed",
          G_CALLBACK (style_changed), fontsel);
  g_signal_connect (fontsel->size_adj, "value_changed",
                    G_CALLBACK (size_changed), fontsel);

  gtk_box_pack_start (GTK_BOX (fontsel), fontsel->family, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (fontsel), fontsel->style, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (fontsel), fontsel->size, FALSE, FALSE, 0);

  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (fontsel->family)->entry),
                      pango_font_description_get_family (fontsel->font_desc));

  gtk_container_set_border_width (GTK_CONTAINER (fontsel), 6);

  gtk_widget_show_all (GTK_WIDGET (fontsel));

  fontsel->realized = TRUE;
}


void
gucharmap_mini_font_selection_class_init (GucharmapMiniFontSelectionClass *clazz)
{
  clazz->changed = NULL;

  gucharmap_mini_font_selection_signals[CHANGED] =
      g_signal_new ("changed", gucharmap_mini_font_selection_get_type (), 
		    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GucharmapMiniFontSelectionClass, changed),
                    NULL, NULL, gucharmap_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
}


void
gucharmap_mini_font_selection_init (GucharmapMiniFontSelection *fontsel)
{
  fontsel->available_faces = NULL;
  fontsel->realized = FALSE;

  gtk_widget_ensure_style (GTK_WIDGET (fontsel));
  fontsel->font_desc = pango_font_description_copy (
          GTK_WIDGET (fontsel)->style->font_desc);


  gtk_widget_ensure_style (GTK_WIDGET (fontsel));
  fontsel->font_desc = pango_font_description_copy (
          GTK_WIDGET (fontsel)->style->font_desc);

  fontsel->size_adj = gtk_adjustment_new (
          pango_font_description_get_size (fontsel->font_desc) / PANGO_SCALE, 
          MIN_FONT_SIZE, MAX_FONT_SIZE, 1, 9, 0);

  /* we don't create any widgets until we're realized */
  g_signal_connect (G_OBJECT (fontsel), "realize", G_CALLBACK (realize), NULL);
}


GtkWidget *
gucharmap_mini_font_selection_new ()
{
  return GTK_WIDGET (g_object_new (gucharmap_mini_font_selection_get_type (), NULL));
}


GType
gucharmap_mini_font_selection_get_type ()
{
  static GType gucharmap_mini_font_selection_type = 0;

  if (gucharmap_mini_font_selection_type == 0)
    {
      static const GTypeInfo gucharmap_mini_font_selection_info =
      {
        sizeof (GucharmapMiniFontSelectionClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gucharmap_mini_font_selection_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GucharmapMiniFontSelection),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gucharmap_mini_font_selection_init
      };

      gucharmap_mini_font_selection_type = g_type_register_static (
              GTK_TYPE_HBOX, "GucharmapMiniFontSelection", 
              &gucharmap_mini_font_selection_info, 0);
    }

  return gucharmap_mini_font_selection_type;
}


/* XXX: should do error checking */
gboolean 
gucharmap_mini_font_selection_set_font_name (GucharmapMiniFontSelection *fontsel,
                                   const gchar *fontname)
{
  pango_font_description_free (fontsel->font_desc);

  fontsel->font_desc = pango_font_description_from_string (fontname);

  if (fontsel->realized)
    {
      gtk_entry_set_text (
              GTK_ENTRY (GTK_COMBO (fontsel->family)->entry), 
              pango_font_description_get_family (fontsel->font_desc));
    
      /* XXX: set_style: figure out how */
    }

  gtk_adjustment_set_value (
          GTK_ADJUSTMENT (fontsel->size_adj), 
          pango_font_description_get_size (fontsel->font_desc) / PANGO_SCALE);

  g_signal_emit (fontsel, gucharmap_mini_font_selection_signals[CHANGED], 0);

  return TRUE;
}



gchar * 
gucharmap_mini_font_selection_get_font_name (GucharmapMiniFontSelection *fontsel)
{
  return pango_font_description_to_string (fontsel->font_desc);
}


/* returns font size in points */
gint
gucharmap_mini_font_selection_get_font_size (GucharmapMiniFontSelection *fontsel)
{
  return pango_font_description_get_size (fontsel->font_desc) / PANGO_SCALE;
}


/* size in points */
void
gucharmap_mini_font_selection_set_font_size (GucharmapMiniFontSelection *fontsel, gint size)
{
  gtk_adjustment_set_value (GTK_ADJUSTMENT (fontsel->size_adj), size);
  set_size (fontsel, size);
}
