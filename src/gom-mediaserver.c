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

#include "gom-dlna-server-device.h"
#include "gom-dleyna-server-device.h"
#include "gom-dleyna-server-manager.h"
#include "gom-mediaserver.h"

static gboolean
dlna_check_server (const gchar *object_path,
                   const gchar *target_udn)
{
  DleynaServerMediaDevice *proxy;
  const char *udn;
  GError *error = NULL;
  proxy =
    dleyna_server_media_device_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       "com.intel.dleyna-server",
                                                       object_path,
                                                       NULL, /* GCancellable */
                                                       &error);

  if (error != NULL)
    {
      g_warning ("Enable to get Device : %s", error->message);
      return FALSE;
    }
  
  udn = dleyna_server_media_device_get_udn (proxy);

  return g_str_equal (target_udn, udn);
}

static gboolean
dlna_get_object_path (const gchar *target_udn,
                      gchar **object_path)
{
  DleynaServerManager *proxy;
  gchar **object_paths, **path;
  GHashTable *servers;
  gboolean ret;
  GError *error = NULL;
  
  ret = FALSE;
  
  proxy = dleyna_server_manager_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        "com.intel.dleyna-server",
                                                        "/com/intel/dLeynaServer",
                                                        NULL, /* GCancellable */
                                                        &error);

  if (error != NULL)
    {
      g_warning ("Couldn't get dleyna proxy : %s", error->message);
      g_error_free (error);
    }
  servers = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);

  dleyna_server_manager_call_get_servers_sync (proxy,
                                               &object_paths,
                                               NULL, /* GCancellable */
                                               &error);
  if (error != NULL)
    {
      g_warning ("Couldn't finish call to GetServers : %s", error->message);
    }

  for (path = object_paths; *path != NULL; path++)
    {
      if (dlna_check_server (*path, target_udn))
        {
          *object_path = g_strdup (*path);
          ret = TRUE;
          break;
        }
    }
  
  return ret;
}


void gom_mediaserver_get_photos (const char *udn,
                                 gboolean dlna_supported)
{
  if (dlna_supported)
    {
      GError *error;
      gchar *object_path;
      if (!dlna_get_object_path (udn, &object_path))
        {
          g_warning ("coudn't find object _path");
          return;
        }
      g_warning ("working : %s", object_path);

      GomDlnaServerDevice *device = gom_dlna_server_device_new_from_object_path (object_path);
      gom_dlna_server_device_search_objects (device);
    }
}
