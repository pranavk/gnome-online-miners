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
#include "gom-dlna-server-manager.h"
#include "gom-dleyna-server-device.h"
#include "gom-dleyna-server-manager.h"
#include "gom-mediaserver.h"

void gom_mediaserver_get_photos (GObject *mngr,
                                 const char *udn,
                                 gboolean dlna_supported,
                                 GList **list)
{
  if (dlna_supported)
    {
      GomDlnaServerManager *dlna_mngr = GOM_DLNA_SERVER_MANAGER (mngr);
      GomDlnaServerDevice *device = gom_dlna_server_manager_get_device (dlna_mngr, udn);
      if (device == NULL)
        {
          g_warning ("Device: %s is currently offline", udn);
          return;
        }
      if (gom_dlna_server_device_get_searchable (device))
        {
          g_print ("Device %s is searchable\n", udn);
          GVariant *out = gom_dlna_server_device_search_objects (device);
          GVariant *var;
          GVariantIter *iter = NULL;
          g_variant_get (out, "aa{sv}", &iter);

          while (g_variant_iter_loop (iter, "@a{sv}", &var))
            {
              GVariantIter *iter1 = NULL;
              GVariant *var1;
              Photo *photo = g_slice_new0 (Photo);
              g_variant_get (var, "a{sv}", &iter1);
              while (g_variant_iter_loop (iter1, "@{sv}", &var1))
                {
                  gchar *str1, *str;
                  GVariant *var2 = NULL;
                  GVariantIter *iter2 = NULL;
                  g_variant_get (var1, "{sv}", &str, &var2);

                  if (g_str_equal (str, "DisplayName"))
                    {
                      g_variant_get (var2, "s", &str1);
                      photo->name = g_strdup (str1);
                    }
                  else if (g_str_equal (str, "URLs"))
                    {
                      g_variant_get (var2, "as", &iter2);
                      g_variant_iter_loop (iter2, "s", &str1);
                      g_variant_iter_free (iter2);
                      photo->url = g_strdup (str1);
                    }
                  else if (g_str_equal (str, "Path"))
                    {
                      g_variant_get (var2, "o", &str1);
                      photo->path = g_strdup (str1);
                    }
                  else
                    {
                      g_variant_get (var2, "s", &str1);
                      photo->mimetype = g_strdup (str1);
                    }

                  g_free (str);
                  g_free (str1);
                }
              *list = g_list_prepend (*list, photo);
            }//outer while
        }//if
      else
        {
          g_print ("Device %s is not searchable\n", udn);
        }
    }
}
