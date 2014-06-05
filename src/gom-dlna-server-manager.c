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

#include <gio/gio.h>

#include "gom-dleyna-server-manager.h"
#include "gom-dlna-server-manager.h"
#include "gom-dlna-server-device.h"


struct _GomDlnaServerManagerPrivate
{
  DleynaServerManager *proxy;
  GHashTable *server;
  GError *error;
};

enum
{
  SERVER_FOUND,
  SERVER_LOST,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (GomDlnaServerManager, gom_dlna_server_manager, G_TYPE_OBJECT);


static GObject *gom_dlna_server_manager_singleton = NULL;



static void
gom_dlna_server_manager_server_new_cb (GObject      *source_object,
                                       GAsyncResult *res,
                                       gpointer      user_data)
{
  GomDlnaServerManager *self = GOM_DLNA_SERVER_MANAGER (user_data);
  GomDlnaServerManagerPrivate *priv = self->priv;
  GomDlnaServerDevice *server;
  const gchar *object_path;
  GError *error = NULL;

  server = gom_dlna_server_device_new_for_bus_finish (res, &error);
  if (error != NULL)
    {
      g_warning ("Unable to load server object: %s", error->message);
      g_propagate_error (&priv->error, error);
      goto out;
    }

  object_path = gom_dlna_server_device_get_object_path (server);
  g_debug ("%s '%s' %s %s", G_STRFUNC,
           gom_dlna_server_device_get_friendly_name (server),
           gom_dlna_server_device_get_udn (server),
           object_path);
  g_hash_table_insert (priv->server, (gpointer) object_path, server);
  g_signal_emit (self, signals[SERVER_FOUND], 0, server);

 out:
  g_object_unref (self);
}


static void
gom_dlna_server_manager_server_found_cb (GomDlnaServerManager *self,
                                         const gchar *object_path,
                                         gpointer *data)
{
  gom_dlna_server_device_new_for_bus (G_BUS_TYPE_SESSION,
                                      G_DBUS_PROXY_FLAGS_NONE,
                                      "com.intel.dleyna-server",
                                      object_path,
                                      NULL, /* GCancellable */
                                      gom_dlna_server_manager_server_new_cb,
                                      g_object_ref (self));
}


static void
gom_dlna_server_manager_server_lost_cb (GomDlnaServerManager *self,
                                        const gchar *object_path,
                                        gpointer *data)
{
  GomDlnaServerManagerPrivate *priv = self->priv;
  GomDlnaServerDevice *server;

  server = GOM_DLNA_SERVER_DEVICE (g_hash_table_lookup (priv->server, object_path));
  g_return_if_fail (server != NULL);

  g_hash_table_steal (priv->server, object_path);
  g_debug ("%s '%s' %s %s", G_STRFUNC,
           gom_dlna_server_device_get_friendly_name (server),
           gom_dlna_server_device_get_udn (server),
           object_path);
  g_signal_emit (self, signals[SERVER_LOST], 0, server);
}


static void
gom_dlna_server_manager_proxy_get_servers_cb (GObject      *source_object,
                                              GAsyncResult *res,
                                              gpointer      user_data)
{
  GomDlnaServerManager *self = user_data;
  GomDlnaServerManagerPrivate *priv = self->priv;
  gchar **object_paths, **path;
  GError *error = NULL;

  dleyna_server_manager_call_get_servers_finish (priv->proxy, &object_paths, res, &error);
  if (error != NULL)
    {
      g_warning ("Unable to fetch the list of available server: %s", error->message);
      g_propagate_error (&priv->error, error);
      goto out;
    }

  for (path = object_paths; *path != NULL; path++)
    gom_dlna_server_manager_server_found_cb (self, *path, NULL);

  g_signal_connect_swapped (priv->proxy, "found-server",
                            G_CALLBACK (gom_dlna_server_manager_server_found_cb),
                            self);
  g_signal_connect_swapped (priv->proxy, "lost-server",
                            G_CALLBACK (gom_dlna_server_manager_server_lost_cb),
                            self);

 out:
  g_strfreev (object_paths);
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
  if (error != NULL)
    {
      g_warning ("Unable to connect to the dLeynaServer.Manager DBus object: %s", error->message);
      g_propagate_error (&priv->error, error);
      goto out;
    }

  g_debug ("%s DLNA server manager initialized", G_STRFUNC);

  dleyna_server_manager_call_get_servers (priv->proxy, NULL,
                                          gom_dlna_server_manager_proxy_get_servers_cb,
                                          g_object_ref (self));

 out:
  g_object_unref (self);
}


static void
gom_dlna_server_manager_finalize (GObject *object)
{
  GomDlnaServerManager *self = GOM_DLNA_SERVER_MANAGER (object);
  GomDlnaServerManagerPrivate *priv = self->priv;

  g_clear_object (&priv->proxy);
  g_clear_pointer (&priv->server, g_hash_table_unref);
  g_clear_error (&priv->error);

  G_OBJECT_CLASS (gom_dlna_server_manager_parent_class)->finalize (object);
}


static GObject *
gom_dlna_server_manager_constructor (GType                  type,
                                     guint                  n_construct_params,
                                     GObjectConstructParam *construct_params)
{
  if (gom_dlna_server_manager_singleton == NULL)
    {
      gom_dlna_server_manager_singleton
        = G_OBJECT_CLASS (gom_dlna_server_manager_parent_class)->constructor (type,
                                                                              n_construct_params,
                                                                              construct_params);
      g_object_add_weak_pointer (gom_dlna_server_manager_singleton,
                                 (gpointer) &gom_dlna_server_manager_singleton);
      return gom_dlna_server_manager_singleton;
    }

  return g_object_ref (gom_dlna_server_manager_singleton);
}


static void
gom_dlna_server_manager_init (GomDlnaServerManager *self)
{
  GomDlnaServerManagerPrivate *priv;

  self->priv = priv = gom_dlna_server_manager_get_instance_private (self);

  dleyna_server_manager_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           "com.intel.dleyna-server",
                                           "/com/intel/dLeynaServer",
                                           NULL, /* GCancellable */
                                           gom_dlna_server_manager_proxy_new_cb,
                                           g_object_ref (self));
  priv->server = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);
}


static void
gom_dlna_server_manager_class_init (GomDlnaServerManagerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->constructor = gom_dlna_server_manager_constructor;
  object_class->finalize = gom_dlna_server_manager_finalize;

  signals[SERVER_FOUND] = g_signal_new ("server-found", G_TYPE_FROM_CLASS (class),
                                        G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, 1, GOM_TYPE_DLNA_SERVER_DEVICE);

  signals[SERVER_LOST] = g_signal_new ("server-lost", G_TYPE_FROM_CLASS (class),
                                       G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                       g_cclosure_marshal_VOID__OBJECT,
                                       G_TYPE_NONE, 1, GOM_TYPE_DLNA_SERVER_DEVICE);
}


GomDlnaServerManager *
gom_dlna_server_manager_dup_singleton (void)
{
  return g_object_new (GOM_TYPE_DLNA_SERVER_MANAGER, NULL);
}


GList *
gom_dlna_server_manager_dup_server (GomDlnaServerManager *self)
{
  GomDlnaServerManagerPrivate *priv = self->priv;
  GList *server;

  server = g_hash_table_get_values (priv->server);
  g_list_foreach (server, (GFunc) g_object_ref, NULL);

  return server;
}


gboolean
gom_dlna_server_manager_is_available (void)
{
  GomDlnaServerManager *self;

  if (gom_dlna_server_manager_singleton == NULL)
    return FALSE;

  self = GOM_DLNA_SERVER_MANAGER (gom_dlna_server_manager_singleton);

  return self->priv->error == NULL;
}
