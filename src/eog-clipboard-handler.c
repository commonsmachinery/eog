/*
 * eog-clipboard-handler.c
 * This file is part of eog
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2010 GNOME Foundation
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "eog-clipboard-handler.h"

#if HAVE_EXEMPI
#include <exempi/xmp.h>
#include <string.h>
#endif

enum {
	PROP_0,
	PROP_PIXBUF,
	PROP_URI,
#if HAVE_EXEMPI
	PROP_XMP,
#endif
};

enum {
	TARGET_PIXBUF,
	TARGET_TEXT,
	TARGET_URI,
#if HAVE_EXEMPI
	TARGET_XMP,
#endif
};

struct _EogClipboardHandlerPrivate {
	GdkPixbuf *pixbuf;
	gchar     *uri;
#if HAVE_EXEMPI
	XmpPtr    xmp;
#endif	
};


#if HAVE_EXEMPI
#define XMP_XML_HEADER "<?xml version='1.0' encoding='UTF-8'?>\n"

#define MIME_TYPE_XMP "application/rdf+xml"

static GdkAtom xmp_atom;

static GdkAtom
get_xmp_atom (void)
{
	if (!xmp_atom) {
		xmp_atom = gdk_atom_intern_static_string(MIME_TYPE_XMP);
	}

	return xmp_atom;
}

#endif

#define EOG_CLIPBOARD_HANDLER_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_CLIPBOARD_HANDLER, EogClipboardHandlerPrivate))

G_DEFINE_TYPE(EogClipboardHandler, eog_clipboard_handler, G_TYPE_INITIALLY_UNOWNED)

static GdkPixbuf*
eog_clipboard_handler_get_pixbuf (EogClipboardHandler *handler)
{
	g_return_val_if_fail (EOG_IS_CLIPBOARD_HANDLER (handler), NULL);

	return handler->priv->pixbuf;
}

static const gchar *
eog_clipboard_handler_get_uri (EogClipboardHandler *handler)
{
	g_return_val_if_fail (EOG_IS_CLIPBOARD_HANDLER (handler), NULL);

	return handler->priv->uri;
}

#ifdef HAVE_EXEMPI

// Does not return a new object, user must xmp_copy() if they need that.
static const XmpPtr
eog_clipboard_handler_get_xmp (EogClipboardHandler *handler)
{
	g_return_val_if_fail (EOG_IS_CLIPBOARD_HANDLER (handler), NULL);

	return handler->priv->xmp;
}
#endif // HAVE_EXEMPI

static void
eog_clipboard_handler_set_pixbuf (EogClipboardHandler *handler, GdkPixbuf *pixbuf)
{
	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (handler));
	g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

	if (handler->priv->pixbuf == pixbuf)
		return;
	
	if (handler->priv->pixbuf)
		g_object_unref (handler->priv->pixbuf);

	handler->priv->pixbuf = g_object_ref (pixbuf);

	g_object_notify (G_OBJECT (handler), "pixbuf");
}

static void
eog_clipboard_handler_set_uri (EogClipboardHandler *handler, const gchar *uri)
{
	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (handler));

	if (handler->priv->uri != NULL)
		g_free (handler->priv->uri);

	handler->priv->uri = g_strdup (uri);
	g_object_notify (G_OBJECT (handler), "uri");
}

#if HAVE_EXEMPI
// Takes over ownership of the xmp object - caller must call xmp_copy() if necessary
static void
eog_clipboard_handler_set_xmp (EogClipboardHandler *handler, XmpPtr xmp)
{
	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (handler));

	if (handler->priv->xmp != NULL)
		xmp_free (handler->priv->xmp);

	handler->priv->xmp = xmp;
	g_object_notify (G_OBJECT (handler), "xmp");
}
#endif // HAVE_EXEMPI

static void
eog_clipboard_handler_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec)
{
	EogClipboardHandler *handler;

	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (object));

	handler = EOG_CLIPBOARD_HANDLER (object);

	switch (property_id) {
	case PROP_PIXBUF:
		g_value_set_object (value,
				    eog_clipboard_handler_get_pixbuf (handler));
		break;
	case PROP_URI:
		g_value_set_string (value,
				    eog_clipboard_handler_get_uri (handler));
		break;
#if HAVE_EXEMPI
	case PROP_XMP:
		g_value_set_pointer (value,
				     eog_clipboard_handler_get_xmp (handler));
		break;
#endif
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eog_clipboard_handler_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec)
{
	EogClipboardHandler *handler;

	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (object));

	handler = EOG_CLIPBOARD_HANDLER (object);

	switch (property_id) {
	case PROP_PIXBUF:
	{
		GdkPixbuf *pixbuf;

		pixbuf = g_value_get_object (value);
		eog_clipboard_handler_set_pixbuf (handler, pixbuf);
		break;
	}
	case PROP_URI:
	{
		const gchar *uri;

		uri = g_value_get_string (value);
		eog_clipboard_handler_set_uri (handler, uri);
		break;
	}
#if HAVE_EXEMPI
	case PROP_XMP:
	{
		XmpPtr xmp;

		xmp = g_value_get_pointer (value);
		eog_clipboard_handler_set_xmp (handler, xmp);
		break;
	}
#endif
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}

}

static void
eog_clipboard_handler_dispose (GObject *obj)
{
	EogClipboardHandlerPrivate *priv;

	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (obj));

	priv = EOG_CLIPBOARD_HANDLER (obj)->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}
	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}
#ifdef HAVE_EXEMPI
	if (priv->xmp) {
		xmp_free(priv->xmp);
		priv->xmp = NULL;
	}
#endif
	
	G_OBJECT_CLASS (eog_clipboard_handler_parent_class)->dispose (obj);
}

static void
eog_clipboard_handler_init (EogClipboardHandler *handler)
{
	handler->priv = EOG_CLIPBOARD_HANDLER_GET_PRIVATE (handler);
}

static void
eog_clipboard_handler_class_init (EogClipboardHandlerClass *klass)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EogClipboardHandlerPrivate));

	g_obj_class->get_property = eog_clipboard_handler_get_property;
	g_obj_class->set_property = eog_clipboard_handler_set_property;
	g_obj_class->dispose = eog_clipboard_handler_dispose;

	g_object_class_install_property (
		g_obj_class, PROP_PIXBUF,
		g_param_spec_object ("pixbuf", NULL, NULL, GDK_TYPE_PIXBUF,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		g_obj_class, PROP_URI,
		g_param_spec_string ("uri", NULL, NULL, NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS));

#ifdef HAVE_EXEMPI
	g_object_class_install_property (
		g_obj_class, PROP_XMP,
		g_param_spec_pointer ("xmp", NULL, NULL, 
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_STATIC_STRINGS));
#endif
}

EogClipboardHandler*
eog_clipboard_handler_new (EogImage *img)
{
	GObject *obj;
	GFile *file;
	GdkPixbuf *pbuf;
	gchar *uri;
#ifdef HAVE_EXEMPI
	XmpPtr *xmp;
#endif	

	g_object_ref (img);
	pbuf = eog_image_get_pixbuf (img);
	file = eog_image_get_file (img);
	uri = g_file_get_uri (file);
#ifdef HAVE_EXEMPI
	// EogImage copies the XMP structure for us
	xmp = eog_image_get_xmp_info (img);
#endif
	obj = g_object_new (EOG_TYPE_CLIPBOARD_HANDLER,
			    "pixbuf", pbuf,
			    "uri", uri,
#ifdef HAVE_EXEMPI
			    "xmp", xmp,
#endif
			    NULL);
	g_free (uri);
	g_object_unref (file);
	g_object_unref (pbuf);
	g_object_unref (img);

	// We keep the XMP pointer in the object and thus don't free
	// it here. Don't know if that is kosher, though.

	return EOG_CLIPBOARD_HANDLER (obj);

}

static void
eog_clipboard_handler_get_func (GtkClipboard *clipboard,
				GtkSelectionData *selection,
				guint info, gpointer owner)
{
	EogClipboardHandler *handler;

	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (owner));

	handler = EOG_CLIPBOARD_HANDLER (owner);

	switch (info) {
	case TARGET_PIXBUF:
	{
		GdkPixbuf *pixbuf = eog_clipboard_handler_get_pixbuf (handler);
		g_object_ref (pixbuf);
		gtk_selection_data_set_pixbuf (selection, pixbuf);
		g_object_unref (pixbuf);
		break;
	}
	case TARGET_TEXT:
	{
		gtk_selection_data_set_text (selection,
					     eog_clipboard_handler_get_uri (handler),
					     -1);
		break;
	}
	case TARGET_URI:
	{
		gchar *uris[2];
		uris[0] = g_strdup (eog_clipboard_handler_get_uri (handler));
		uris[1] = NULL;

		gtk_selection_data_set_uris (selection, uris);
		g_free (uris[0]);
		break;
	}

#ifdef HAVE_EXEMPI
	case TARGET_XMP:
	{
		// Serialize the XMP into XML
		XmpStringPtr str;
		const char *xmp_element;
		gchar *xml_doc = NULL;
		gboolean serialized = FALSE;

		str = xmp_string_new ();
		if (str) {
			serialized = xmp_serialize (
				eog_clipboard_handler_get_xmp (handler),
				str,
				(XMP_SERIAL_OMITPACKETWRAPPER
				 | XMP_SERIAL_ENCODEUTF8),
				0);
		}

		if (serialized) {
			xmp_element = xmp_string_cstr (str);
			xml_doc = g_strconcat (XMP_XML_HEADER, xmp_element, NULL);
		}

		if (xml_doc) {
			gtk_selection_data_set (
				selection, get_xmp_atom (), 8,
				(const guchar*) xml_doc, strlen (xml_doc));
			
			g_free (xml_doc);
			xml_doc = NULL;
		}
		
		if (str) {
			xmp_string_free (str);
			str = NULL;
		}
		
#endif // HAVE_EXEMPI
		
		break;
	}

	default:
		g_return_if_reached ();
	}

}

static void
eog_clipboard_handler_clear_func (GtkClipboard *clipboard, gpointer owner)
{
	g_return_if_fail (EOG_IS_CLIPBOARD_HANDLER (owner));

	g_object_unref (G_OBJECT (owner));
}

void
eog_clipboard_handler_copy_to_clipboard (EogClipboardHandler *handler,
					 GtkClipboard *clipboard)
{
	GtkTargetList *tlist;
	GtkTargetEntry *targets;
	gint n_targets = 0;
	gboolean set = FALSE;

	tlist = gtk_target_list_new (NULL, 0);

	if (handler->priv->pixbuf != NULL)
		gtk_target_list_add_image_targets (tlist, TARGET_PIXBUF, TRUE);

	if (handler->priv->uri != NULL) {
		gtk_target_list_add_text_targets (tlist, TARGET_TEXT);
		gtk_target_list_add_uri_targets (tlist, TARGET_URI);
	}

#ifdef HAVE_EXEMPI
	if (handler->priv->xmp != NULL) {
		gtk_target_list_add (tlist, get_xmp_atom (), 0, TARGET_XMP);
	}
#endif


	targets = gtk_target_table_new_from_list (tlist, &n_targets);

	// We need to take ownership here if nobody else did
	g_object_ref_sink (handler);

	if (n_targets > 0) {
		set = gtk_clipboard_set_with_owner (clipboard,
						    targets, n_targets,
						    eog_clipboard_handler_get_func,
						    eog_clipboard_handler_clear_func,
						    G_OBJECT (handler));

	} 
	
	if (!set) {
		gtk_clipboard_clear (clipboard);
		g_object_unref (handler);
	}

	gtk_target_table_free (targets, n_targets);
	gtk_target_list_unref (tlist);
}

