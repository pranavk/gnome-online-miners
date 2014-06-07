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
                                 gboolean dlna_supported)
{
  if (dlna_supported)
    {
      g_print (udn);
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
        }else
        {
          g_print ("Device %s is not searchable\n", udn);
        }
    }
}

