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

#ifndef GOM_DLNA_SERVER_MANAGER_H
#define GOM_DLNA_SERVER_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GOM_TYPE_DLNA_SERVER_MANAGER (gom_dlna_server_manager_get_type ())

#define GOM_DLNA_SERVER_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
   GOM_TYPE_DLNA_SERVER_MANAGER, GomDlnaServerManager))

#define GOM_DLNA_SERVER_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
   GOM_TYPE_DLNA_SERVER_MANAGER, GomDlnaServerManagerClass))

#define GOM_IS_DLNA_SERVER_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
   GOM_TYPE_DLNA_SERVER_MANAGER))

#define GOM_IS_DLNA_SERVER_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
   GOM_TYPE_DLNA_SERVER_MANAGER))

#define GOM_DLNA_SERVER_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
   GOM_TYPE_DLNA_SERVER_MANAGER, GomDlnaServerManagerClass))

typedef struct _GomDlnaServerManager        GomDlnaServerManager;
typedef struct _GomDlnaServerManagerClass   GomDlnaServerManagerClass;
typedef struct _GomDlnaServerManagerPrivate GomDlnaServerManagerPrivate;

struct _GomDlnaServerManager
{
  GObject parent_instance;
  GomDlnaServerManagerPrivate *priv;
};

struct _GomDlnaServerManagerClass
{
  GObjectClass parent_class;
};

GType                       gom_dlna_server_manager_get_type      (void) G_GNUC_CONST;

GomDlnaServerManager *gom_dlna_server_manager_dup_singleton (void);

GList                      *gom_dlna_server_manager_dup_server (GomDlnaServerManager *self);

gboolean                    gom_dlna_server_manager_is_available  (void);

G_END_DECLS

#endif /* GOM_DLNA_SERVER_MANAGER_H */
