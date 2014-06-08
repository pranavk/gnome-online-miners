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

#include "gom-dlna-server-manager.h"

#include <gio/gio.h>

#include "gom-dleyna-server-manager.h"
#include "gom-dlna-server-device.h"

struct _GomDlnaServerManagerPrivate
{
  DleynaServerManager *proxy;
  GHashTable *server;
  GHashTable *udn_to_server;

  gboolean got_error;
};

enum
  {
    SERVER_FOUND,
    SERVER_LOST,
    LAST_SIGNAL
  };

static guint signals[LAST_SIGNAL] = { 0 };

static GObject *gom_dlna_server_manager_singleton = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (GomDlnaServerManager, gom_dlna_server_manager, G_TYPE_OBJECT)

static void
gom_dlna_server_manager_dispose (GObject *object)
{
  GomDlnaServerManager *self = GOM_DLNA_SERVER_MANAGER (object);
  GomDlnaServerManagerPrivate *priv = self->priv;

  g_clear_object (&priv->proxy);
  g_clear_pointer (&priv->server, g_hash_table_unref);

  G_OBJECT_CLASS (gom_dlna_server_manager_parent_class)->dispose (object);
}


static void
gom_dlna_server_manager_server_found_cb (GomDlnaServerManager *self,
                                         const gchar             *object_path,
                                         gpointer                *data)
{
  GomDlnaServerManagerPrivate *priv = self->priv;
  GomDlnaServerDevice *server;
  const gchar *udn;

  server = gom_dlna_server_device_new_from_object_path (object_path);

  udn = gom_dlna_server_device_get_udn (server);

  g_hash_table_insert (priv->server, (gpointer) object_path, g_object_ref (server));
  g_hash_table_insert (priv->udn_to_server, (gpointer) udn, g_object_ref (server));

  g_signal_emit (self, signals[SERVER_FOUND], 0, server);
}

static void
gom_dlna_server_manager_server_lost_cb (GomDlnaServerManager *self,
                                        const gchar             *object_path,
                                        gpointer                *data)
{
  GomDlnaServerManagerPrivate *priv = self->priv;
  GomDlnaServerDevice *server;
  const gchar *udn;


  server = GOM_DLNA_SERVER_DEVICE (g_hash_table_lookup (priv->server, object_path));
  g_return_if_fail (server != NULL);

  udn = gom_dlna_server_device_get_udn (server),
  g_hash_table_steal (priv->server, udn);
  g_hash_table_steal (priv->server, object_path);

  g_signal_emit (self, signals[SERVER_LOST], 0, server);
  g_object_unref (server);
}

static void
gom_dlna_server_manager_proxy_get_server_cb (GObject      *source_object,
                                             GAsyncResult *res,
                                             gpointer      user_data)
{
  GomDlnaServerManager *self = user_data;
  GomDlnaServerManagerPrivate *priv = self->priv;
  gchar **object_paths, **path;
  GError *error = NULL;

  dleyna_server_manager_call_get_servers_finish (priv->proxy, &object_paths, res, &error);
  if (error != NULL) {
    g_warning (error->message);
    g_error_free (error);
    priv->got_error = TRUE;
    return;
  }

  for (path = object_paths; *path != NULL; path++) {
    gom_dlna_server_manager_server_found_cb (self, *path, NULL);
  }

  g_strfreev (object_paths);

  /* Release the ref taken in gom_dlna_manager_proxy_new_for_bus() */
  g_object_unref (self);
}

static void
gom_dlna_server_manager_proxy_new_cb (GObject      *source_object,
                                      GAsyncResult *res,
                                      gpointer      user_data)
{
  GomDlnaServerManager *self = user_data;
  GomDlnaServerManagerPrivate *priv = self->priv;
  GError *error = NULL;

  priv->proxy = dleyna_server_manager_proxy_new_for_bus_finish (res, &error);
  if (error != NULL) {
    g_warning ("Unable to connect to the dlnaServer.Manager DBus object: %s", error->message);
    g_error_free (error);
    priv->got_error = TRUE;
    return;
  }

  g_object_connect (priv->proxy,
                    "swapped-object-signal::found-server",
                    gom_dlna_server_manager_server_found_cb, self,
                    "swapped-object-signal::lost-server",
                    gom_dlna_server_manager_server_lost_cb, self,
                    NULL);

  dleyna_server_manager_call_get_servers (priv->proxy, NULL,
                                          gom_dlna_server_manager_proxy_get_server_cb,
                                          self);
}

static GObject *
gom_dlna_server_manager_constructor (GType                  type,
                                     guint                  n_construct_params,
                                     GObjectConstructParam *construct_params)
{
  if (gom_dlna_server_manager_singleton != NULL) {
    return g_object_ref (gom_dlna_server_manager_singleton);
  }

  gom_dlna_server_manager_singleton =
    G_OBJECT_CLASS (gom_dlna_server_manager_parent_class)->constructor (type, n_construct_params,
                                                                        construct_params);

  g_object_add_weak_pointer (gom_dlna_server_manager_singleton, (gpointer) &gom_dlna_server_manager_singleton);

  return gom_dlna_server_manager_singleton;
}

static void
gom_dlna_server_manager_init (GomDlnaServerManager *self)
{
  GomDlnaServerManagerPrivate *priv;

  self->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GOM_TYPE_DLNA_SERVER_MANAGER,
                                                   GomDlnaServerManagerPrivate);

  dleyna_server_manager_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           "com.intel.dleyna-server",
                                           "/com/intel/dLeynaServer",
                                           NULL,
                                           gom_dlna_server_manager_proxy_new_cb,
                                           g_object_ref (self));
  priv->server = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);
  priv->udn_to_server = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);
}

static void
gom_dlna_server_manager_class_init (GomDlnaServerManagerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->constructor = gom_dlna_server_manager_constructor;
  object_class->dispose = gom_dlna_server_manager_dispose;

  signals[SERVER_FOUND] = g_signal_new ("server-found", G_TYPE_FROM_CLASS (class),
                                        G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, 1, GOM_TYPE_DLNA_SERVER_DEVICE);

  signals[SERVER_LOST] = g_signal_new ("server-lost", G_TYPE_FROM_CLASS (class),
                                       G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                       g_cclosure_marshal_VOID__OBJECT,
                                       G_TYPE_NONE, 1, GOM_TYPE_DLNA_SERVER_DEVICE);
}

GomDlnaServerDevice *
gom_dlna_server_manager_get_device (GomDlnaServerManager *self,
                                    const gchar *udn)
{
  GomDlnaServerManagerPrivate *priv = self->priv;
  GomDlnaServerDevice *device;

  device = GOM_DLNA_SERVER_DEVICE (g_hash_table_lookup (priv->udn_to_server, udn));
  if (device == NULL)
    return NULL;

  return g_object_ref (device);
}

GomDlnaServerManager *
gom_dlna_server_manager_dup_singleton (void)
{
  return g_object_new (GOM_TYPE_DLNA_SERVER_MANAGER, NULL);
}

gboolean
gom_dlna_server_manager_is_available (void)
{
  GomDlnaServerManager *self;

  if (gom_dlna_server_manager_singleton == NULL)
    return FALSE;

  self = GOM_DLNA_SERVER_MANAGER (gom_dlna_server_manager_singleton);

  return self->priv->got_error == FALSE;
}
