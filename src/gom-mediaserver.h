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

#ifndef _MEDIA_SERVER_H_
#define _MEDIA_SERVER_H_

typedef struct {
  const gchar *name;
  const gchar *url;
  const gchar *mimetype;
  const gchar *path;
} Photo;

void gom_mediaserver_get_photos (GObject *mngr,
                                 const gchar *udn,
                                 gboolean dlnaSupported,
                                 GList **list);

#endif
