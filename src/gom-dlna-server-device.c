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

#include "config.h"

#include "gom-dleyna-server-device.h"
#include "gom-dleyna-server-mediacontainer2.h"
#include "gom-dlna-server-device.h"

struct _GomDlnaServerDevicePrivate
{
  DleynaServerMediaDevice *device;
  DleynaServerMediaContainer2 *container;
  gchar *udn;
  gchar *object_path;
  gchar *friendly_name;
};


enum{
  PROP_0,
  PROP_OBJECT_PATH,
  PROP_NAME,
  PROP_UDN
};

G_DEFINE_TYPE_WITH_PRIVATE (GomDlnaServerDevice, gom_dlna_server_device, G_TYPE_OBJECT)

static void
gom_dlna_server_device_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  GomDlnaServerDevice *self = GOM_DLNA_SERVER_DEVICE (object);
  GomDlnaServerDevicePrivate *priv = self->priv;
  
  switch (prop_id)
    {
    case PROP_UDN:
      g_value_set_string (value, priv->udn);
      break;

    case PROP_OBJECT_PATH:
      g_value_set_string (value, priv->object_path);
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gom_dlna_server_device_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
  GomDlnaServerDevice *self = GOM_DLNA_SERVER_DEVICE (object);
  GomDlnaServerDevicePrivate *priv = self->priv;
  GError *error = NULL;
  
  switch (prop_id)
    {
    case PROP_OBJECT_PATH:
      priv->object_path = g_value_dup_string (value);
      priv->device =
        dleyna_server_media_device_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                           G_DBUS_PROXY_FLAGS_NONE,
                                                           "com.intel.dleyna-server",
                                                           priv->object_path,
                                                           NULL, /* GCancellable */
                                                           &error);
      if (error != NULL)
        {
          g_warning ("Unable to get device : %s", error->message);
          g_error_free (error);
        }

      priv->container =
        dleyna_server_media_container2_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                               G_DBUS_PROXY_FLAGS_NONE,
                                                               "com.intel.dleyna-server",
                                                               priv->object_path,
                                                               NULL,
                                                               &error);

      if (error != NULL)
        {
          g_warning ("Error while getting proxy for Mediacontainer2 : %s",
                     error->message);
          g_error_free (error);
        }

      priv->friendly_name = dleyna_server_media_device_get_friendly_name (priv->device);
      priv->udn = dleyna_server_media_device_get_udn (priv->device);
      
      break;

    case PROP_NAME:
      priv->friendly_name = g_value_dup_string (value);
      break;

    case PROP_UDN:
      priv->udn = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

const gchar *
gom_dlna_server_device_get_object_path (GomDlnaServerDevice *self)
{
  return self->priv->object_path;
}

const gchar *
gom_dlna_server_device_get_friendly_name (GomDlnaServerDevice *self)
{
  GomDlnaServerDevicePrivate *priv = self->priv;

  return dleyna_server_media_device_get_friendly_name (priv->device);
}

gboolean
gom_dlna_server_device_search_objects (GomDlnaServerDevice *self)
{
  GomDlnaServerDevicePrivate *priv = self->priv;
  GError *error = NULL;

  GVariant *out;
  gchar *query = g_strdup_printf ("MIMEType = \"image/jpeg\"");
  gchar *filter[] = {"*"};
  dleyna_server_media_container2_call_search_objects_sync (priv->container,
                                                           query,
                                                           0,
                                                           0,
                                                           filter,
                                                           &out,
                                                           NULL,
                                                           &error);
  if (error != NULL)
    g_warning ("error found");
  g_warning ("success");
  g_warning (g_variant_get_data (out));
  g_free (query);
}

const gchar *
gom_dlna_server_device_get_udn (GomDlnaServerDevice *self)
{
  GomDlnaServerDevicePrivate *priv = self->priv;

  return dleyna_server_media_device_get_udn (priv->device);
}

const gchar *
gom_dlna_server_device_get_mimetype (GomDlnaServerDevice *self)
{
  GomDlnaServerDevicePrivate *priv = self->priv;
  return dleyna_server_media_container2_get_mimetype (priv->container);
}

static void
gom_dlna_server_device_finalize (GObject *object)
{
  GomDlnaServerDevice *self = GOM_DLNA_SERVER_DEVICE (object);
  GomDlnaServerDevicePrivate *priv = self->priv;

  g_clear_object (&priv->device);
  g_clear_object (&priv->container);
  g_free (priv->friendly_name);
  g_free (priv->object_path);

  G_OBJECT_CLASS (gom_dlna_server_device_parent_class)->finalize (object);
}

static void
gom_dlna_server_device_init (GomDlnaServerDevice *self)
{
  self->priv = gom_dlna_server_device_get_instance_private (self);
}

GomDlnaServerDevice *
gom_dlna_server_device_new_from_object_path (const gchar *object_path)
{
  return g_object_new (GOM_TYPE_DLNA_SERVER_DEVICE, "object-path", object_path, NULL);
}

static void
gom_dlna_server_device_class_init (GomDlnaServerDeviceClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = gom_dlna_server_device_finalize;
  gobject_class->get_property = gom_dlna_server_device_get_property;
  gobject_class->set_property = gom_dlna_server_device_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        "Object Path",
                                                        "The object path the proxy is for",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  g_object_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("friendly-name",
                                                        "Friendly Name",
                                                        "The well-known name of the service",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

    g_object_class_install_property (gobject_class,
                                     PROP_UDN,
                                     g_param_spec_string ("udn",
                                                          "UDN",
                                                          "UDN of the device",
                                                          NULL,
                                                          G_PARAM_READWRITE |
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_BLURB |
                                                          G_PARAM_STATIC_NICK));
}

