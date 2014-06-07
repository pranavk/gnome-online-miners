/*
 * GNOME Online Miners - crawls through your online content
 * Copyright (c) 2014 Pranav Kant.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Author: Pranav Kant <pranav913@gmail.com>
 * 
 */

#ifndef GOM_DLNA_SERVER_DEVICE_H
#define GOM_DLNA_SERVER_DEVICE_H

#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GOM_TYPE_DLNA_SERVER_DEVICE (gom_dlna_server_device_get_type ())

#define GOM_DLNA_SERVER_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
   GOM_TYPE_DLNA_SERVER_DEVICE, GomDlnaServerDevice))

#define GOM_DLNA_SERVER_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
   GOM_TYPE_DLNA_SERVER_DEVICE, GomDlnaServerDeviceClass))

#define GOM_IS_DLNA_SERVER_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
   GOM_TYPE_DLNA_SERVER_DEVICE))

#define GOM_IS_DLNA_SERVER_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
   GOM_TYPE_DLNA_SERVER_DEVICE))

#define GOM_DLNA_SERVER_DEVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
   GOM_TYPE_DLNA_SERVER_DEVICE, GomDlnaServerDeviceClass))

typedef struct _GomDlnaServerDevice        GomDlnaServerDevice;
typedef struct _GomDlnaServerDeviceClass   GomDlnaServerDeviceClass;
typedef struct _GomDlnaServerDevicePrivate GomDlnaServerDevicePrivate;

struct _GomDlnaServerDevice
{
  GObject parent_instance;
  GomDlnaServerDevicePrivate *priv;
};

struct _GomDlnaServerDeviceClass
{
  GObjectClass parent_class;
};

GType                 gom_dlna_server_device_get_type           (void) G_GNUC_CONST;

GomDlnaServerDevice  *gom_dlna_server_device_new_from_object_path (const gchar *object_path);

const gchar          *gom_dlna_server_device_get_object_path    (GomDlnaServerDevice  *server_device);
gboolean          gom_dlna_server_device_get_serchable    (GomDlnaServerDevice  *server_device);

gboolean             gom_dlna_server_device_search_objects    (GomDlnaServerDevice  *device);

const gchar          *gom_dlna_server_device_get_friendly_name  (GomDlnaServerDevice  *self);

const gchar          *gom_dlna_server_device_get_udn            (GomDlnaServerDevice  *self);

G_END_DECLS

#endif /* GOM_DLNA_SERVER_DEVICE_H */
