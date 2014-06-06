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

#include <goa/goa.h>

#include "gom-mediaserver-miner.h"
#include "gom-mediaserver-manager.h"
#include "gom-mediaserver.h"

#define MINER_IDENTIFIER "gd:media_server:miner:a4a47a3e-eb55-11e3-b983-14feb59cfa0e"

struct _GomMediaServerMinerPrivate {
  GomMediaServerManager *dlna_mngr;
};

G_DEFINE_TYPE_WITH_PRIVATE (GomMediaServerMiner, gom_media_server_miner, GOM_TYPE_MINER)

static void
query_media_server (GomAccountMinerJob *job,
                    GError **error)
{
  GomMediaServerMiner *miner = GOM_MEDIA_SERVER_MINER (job->miner);
  GomMediaServerMinerPrivate *priv = miner->priv;

  gboolean dlna_supported;
  
  GoaObject *object = GOA_OBJECT (job->service);
  const char *udn;

  GoaMediaServer *mediaserver = goa_object_peek_media_server (object);
  
  g_object_get (G_OBJECT (mediaserver),
                "dlna-supported", &dlna_supported,
                NULL);
  g_object_get (G_OBJECT (mediaserver),
                "udn", &udn);

  gom_mediaserver_get_photos (udn, dlna_supported);

}

static GObject *
create_service (GomMiner *self,
                GoaObject *object)
{
  GomMediaServerMiner *miner = GOM_MEDIA_SERVER_MINER (self);
  GomMediaServerMinerPrivate *priv = miner->priv;

  return g_object_ref (object);
}

static void
gom_media_server_miner_finalize (GObject *object)
{
  GomMediaServerMinerPrivate *priv = GOM_MEDIA_SERVER_MINER (object)->priv;

  g_clear_object (&priv->dlna_mngr);

  (G_OBJECT_CLASS (gom_media_server_miner_parent_class)->finalize) (object);
}

static void
gom_media_server_miner_init (GomMediaServerMiner *miner)
{
  miner->priv = gom_media_server_miner_get_instance_private (miner);
}

static void
gom_media_server_miner_class_init (GomMediaServerMinerClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GomMinerClass *miner_class = GOM_MINER_CLASS (klass);

  oclass->finalize = gom_media_server_miner_finalize;
  
  miner_class->goa_provider_type = "media_server";
  miner_class->miner_identifier = MINER_IDENTIFIER;
  miner_class->version = 3;

  miner_class->create_service = create_service;
  miner_class->query = query_media_server;
}
