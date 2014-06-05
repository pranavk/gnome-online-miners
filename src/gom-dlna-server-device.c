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
#include "gom-dlna-server-device.h"

struct _GomDlnaServerDevicePrivate
{
  DleynaServerMediaDevice *device;
  GBusType bus_type;
  GDBusProxyFlags flags;
  GHashTable *urls_to_item;
  gchar *object_path;
  gchar *well_known_name;
};

enum
  {
    PROP_0,
    PROP_BUS_TYPE,
    PROP_FLAGS,
    PROP_OBJECT_PATH,
    PROP_SHARED_COUNT,
    PROP_WELL_KNOWN_NAME,
  };

static void gom_dlna_server_device_async_initable_iface_init (GAsyncInitableIface *iface);


G_DEFINE_TYPE_WITH_CODE (GomDlnaServerDevice, gom_dlna_server_device, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GomDlnaServerDevice)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                gom_dlna_server_device_async_initable_iface_init));


#define RETURN_ON_ERROR(task, error, msg)                       \
  G_STMT_START {                                                \
    if (error != NULL)                                          \
      {                                                         \
        g_debug ("%s: %s: %s", G_STRFUNC, msg, error->message); \
        g_task_return_error (task, error);                      \
        g_object_unref (task);                                  \
        return;                                                 \
      }                                                         \
  } G_STMT_END




static void
gom_dlna_server_device_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  GomDlnaServerDevice *self = GOM_DLNA_SERVER_DEVICE (object);

  switch (prop_id)
    {
    case PROP_SHARED_COUNT:
      g_value_set_uint (value, gom_dlna_server_device_get_shared_count (self));
      break;

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

  switch (prop_id)
    {
    case PROP_BUS_TYPE:
      priv->bus_type = g_value_get_enum (value);
      break;

    case PROP_FLAGS:
      priv->flags = g_value_get_flags (value);
      break;

    case PROP_OBJECT_PATH:
      priv->object_path = g_value_dup_string (value);
      break;

    case PROP_WELL_KNOWN_NAME:
      priv->well_known_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


GomDlnaServerDevice*
gom_dlna_server_device_new_for_bus_finish (GAsyncResult *result,
                                           GError **error)
{
  GObject *object, *source_object;
  GError *err = NULL;

  source_object = g_async_result_get_source_object (result);
  object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), result, &err);
  g_object_unref (source_object);

  if (err != NULL)
    {
      g_clear_object (&object);
      g_propagate_error (error, err);
      return NULL;
    }

  return GOM_DLNA_SERVER_DEVICE (object);
}

void
gom_dlna_server_device_new_for_bus (GBusType bus_type,
                                    GDBusProxyFlags flags,
                                    const gchar *well_known_name,
                                    const gchar *object_path,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
  g_async_initable_new_async (GOM_TYPE_DLNA_SERVER_DEVICE,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "bus-type", bus_type,
                              "flags", flags,
                              "object-path", object_path,
                              "well-known-name", well_known_name,
                              NULL);
}



static void
gom_dlna_server_device_get_icon_cb (GObject *source_object,
                                    GAsyncResult *res,
                                    gpointer user_data)
{
  GTask *task = G_TASK (user_data);
  GInputStream *icon_stream;
  GdkPixbuf *pixbuf;
  GtkIconSize size;
  gchar *icon_data;
  gint height = -1;
  gint width = -1;
  gsize icon_data_size;
  GError *error = NULL;

  /* The icon data is forced to be a GVariant since the GDBus bindings
   * assume bytestrings (type 'ay') to be nul-terminated and thus do
   * not return the length of the buffer.
   */
  dleyna_server_media_device_call_get_icon_finish (DLEYNA_SERVER_MEDIA_DEVICE (source_object),
                                                   &icon_data, NULL, res, &error);
  RETURN_ON_ERROR (task, error, "Failed to call the GetIcon method");

  size = (GtkIconSize) GPOINTER_TO_INT (g_task_get_task_data (task));
  gtk_icon_size_lookup (size, &width, &height);
  icon_data_size = sizeof (*icon_data);
  
  icon_stream = g_memory_input_stream_new_from_data (icon_data, icon_data_size, NULL);
  pixbuf = gdk_pixbuf_new_from_stream_at_scale (icon_stream,
                                                width,
                                                height,
                                                TRUE,
                                                g_task_get_cancellable (task),
                                                &error);
  g_object_unref (icon_stream);

  RETURN_ON_ERROR (task, error, "Failed to parse icon data");
  g_task_return_pointer (task, pixbuf, g_object_unref);

  g_object_unref (task);
}

GdkPixbuf *
gom_dlna_server_device_get_icon_finish (GomDlnaServerDevice *self,
                                        GAsyncResult *res,
                                        GError **error)
{
  g_return_val_if_fail (g_task_is_valid (res, self), NULL);

  return GDK_PIXBUF (g_task_propagate_pointer (G_TASK (res), error));
}

void
gom_dlna_server_device_get_icon (GomDlnaServerDevice *self,
                                 const gchar *requested_mimetype,
                                 const gchar *resolution,
                                 GtkIconSize size,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data)
{
  GomDlnaServerDevicePrivate *priv = self->priv;
  GTask *task;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, GINT_TO_POINTER (size), NULL);

  dleyna_server_media_device_call_get_icon (priv->device, requested_mimetype, resolution,
                                            cancellable, gom_dlna_server_device_get_icon_cb,
                                            task);
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

const gchar *
gom_dlna_server_device_get_udn (GomDlnaServerDevice *self)
{
  GomDlnaServerDevicePrivate *priv = self->priv;

  return dleyna_server_media_device_get_udn (priv->device);
}

guint
gom_dlna_server_device_get_shared_count (GomDlnaServerDevice *self)
{
  GomDlnaServerDevicePrivate *priv = self->priv;

  return g_hash_table_size (priv->urls_to_item);
}


static void
gom_dlna_server_device_proxy_new_cb (GObject *source_object,
                                     GAsyncResult *res,
                                     gpointer user_data)
{
  GTask *init_task = G_TASK (user_data);
  GomDlnaServerDevice *self;
  GError *error = NULL;

  self = GOM_DLNA_SERVER_DEVICE (g_task_get_source_object (init_task));

  self->priv->device = dleyna_server_media_device_proxy_new_for_bus_finish (res, &error);
  RETURN_ON_ERROR (init_task, error, "Unable to load the MediaDevice interface");

  g_task_return_boolean (init_task, TRUE);
  g_object_unref (init_task);
}


static void
gom_dlna_server_device_init_async (GAsyncInitable *initable,
                                   int io_priority,
                                   GCancellable *cancellable,
                                   GAsyncReadyCallback callback,
                                   gpointer user_data)
{
  GomDlnaServerDevice *self = GOM_DLNA_SERVER_DEVICE (initable);
  GomDlnaServerDevicePrivate *priv = self->priv;
  GTask *init_task;

  init_task = g_task_new (initable, cancellable, callback, user_data);
  g_task_set_priority (init_task, io_priority);

  dleyna_server_media_device_proxy_new_for_bus (priv->bus_type,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                priv->well_known_name,
                                                priv->object_path,
                                                cancellable,
                                                gom_dlna_server_device_proxy_new_cb,
                                                init_task);
}


static void
gom_dlna_server_device_async_initable_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = gom_dlna_server_device_init_async;
}

static void
gom_dlna_server_device_dispose (GObject *object)
{
  GomDlnaServerDevice *self = GOM_DLNA_SERVER_DEVICE (object);
  GomDlnaServerDevicePrivate *priv = self->priv;

  g_clear_object (&priv->device);
  g_clear_pointer (&priv->urls_to_item, g_hash_table_unref);

  G_OBJECT_CLASS (gom_dlna_server_device_parent_class)->dispose (object);
}


static void
gom_dlna_server_device_finalize (GObject *object)
{
  GomDlnaServerDevice *self = GOM_DLNA_SERVER_DEVICE (object);
  GomDlnaServerDevicePrivate *priv = self->priv;

  g_free (priv->well_known_name);
  g_free (priv->object_path);

  G_OBJECT_CLASS (gom_dlna_server_device_parent_class)->finalize (object);
}

static void
gom_dlna_server_device_init (GomDlnaServerDevice *self)
{
  GomDlnaServerDevicePrivate *priv;

  self->priv = gom_dlna_server_device_get_instance_private (self);
  priv = self->priv;

  priv->urls_to_item = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}


static void
gom_dlna_server_device_class_init (GomDlnaServerDeviceClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = gom_dlna_server_device_dispose;
  gobject_class->finalize = gom_dlna_server_device_finalize;
  gobject_class->get_property = gom_dlna_server_device_get_property;
  gobject_class->set_property = gom_dlna_server_device_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_BUS_TYPE,
                                   g_param_spec_enum ("bus-type",
                                                      "Bus Type",
                                                      "The bus to connect to, defaults to the session one",
                                                      G_TYPE_BUS_TYPE,
                                                      G_BUS_TYPE_SESSION,
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB |
                                                      G_PARAM_STATIC_NICK));

  g_object_class_install_property (gobject_class,
                                   PROP_FLAGS,
                                   g_param_spec_flags ("flags",
                                                       "Flags",
                                                       "Proxy flags",
                                                       G_TYPE_DBUS_PROXY_FLAGS,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        "Object Path",
                                                        "The object path the proxy is for",
                                                        NULL,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  g_object_class_install_property (gobject_class,
                                   PROP_SHARED_COUNT,
                                   g_param_spec_uint ("shared-count",
                                                      "Shared Count",
                                                      "The number of shared items",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB |
                                                      G_PARAM_STATIC_NICK));

  g_object_class_install_property (gobject_class,
                                   PROP_WELL_KNOWN_NAME,
                                   g_param_spec_string ("well-known-name",
                                                        "Well-Known Name",
                                                        "The well-known name of the service",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));
}

