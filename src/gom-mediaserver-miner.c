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
#include "gom-mediaserver.h"
#include "gom-dlna-server-manager.h"

#define MINER_IDENTIFIER "gd:media_server:miner:a4a47a3e-eb55-11e3-b983-14feb59cfa0e"

struct _GomMediaServerMinerPrivate {
  GObject *mngr;
};

G_DEFINE_TYPE_WITH_PRIVATE (GomMediaServerMiner, gom_media_server_miner, GOM_TYPE_MINER)


static gboolean
account_miner_job_process_photo (GomAccountMinerJob *job,
                                 Photo *photo,
                                 const gchar *creator,
                                 GError **error)
{
  GTimeVal new_mtime;
  const gchar *photo_id;
  const gchar *photo_name;
  const gchar *mimetype;
  const gchar *photo_link;
  gchar *identifier;
  const gchar *class = "nmm:Photo";
  gchar *resource = NULL;
  gboolean resource_exists, mtime_changed;
  gchar *contact_resource;

  gchar **ar = g_strsplit_set (photo->path, "/", -1);
  gchar *l;
  while (*ar != NULL){
    l = *ar;
    ar++;
  }
  
  photo_id = g_strdup (l);

  photo_link = photo->url;
  photo_name = photo->name;

  mimetype = photo->mimetype;
  
  identifier = g_strdup_printf ("mediaserver:%s", photo_id);

  /* remove from the list of the previous resources */
  g_hash_table_remove (job->previous_resources, identifier);

  resource = gom_tracker_sparql_connection_ensure_resource
    (job->connection,
     job->cancellable, error,
     &resource_exists,
     job->datasource_urn, identifier,
     "nfo:RemoteDataObject", class, NULL);

  if (*error != NULL)
    goto out;

  gom_tracker_update_datasource (job->connection, job->datasource_urn,
                                 resource_exists, identifier, resource,
                                 job->cancellable, error);
  if (*error != NULL)
    goto out;

  /* the resource changed - just set all the properties again */
  gom_tracker_sparql_connection_insert_or_replace_triple
    (job->connection,
     job->cancellable, error,
     job->datasource_urn, resource,
     "nie:url", photo_link);

  if (*error != NULL)
    goto out;

  gom_tracker_sparql_connection_insert_or_replace_triple
    (job->connection,
     job->cancellable, error,
     job->datasource_urn, resource,
     "nie:mimeType", mimetype);

  if (*error != NULL)
    goto out;

  gom_tracker_sparql_connection_insert_or_replace_triple
    (job->connection,
     job->cancellable, error,
     job->datasource_urn, resource,
     "nie:title", photo_name);

  if (*error != NULL)
    goto out;

  contact_resource = gom_tracker_utils_ensure_contact_resource
    (job->connection,
     job->cancellable, error,
     job->datasource_urn, creator);

  if (*error != NULL)
    goto out;

  gom_tracker_sparql_connection_insert_or_replace_triple
    (job->connection,
     job->cancellable, error,
     job->datasource_urn, resource,
     "nco:creator", contact_resource);

  g_free (contact_resource);

  if (*error != NULL)
    goto out;

 out:
  g_free (resource);
  g_free (identifier);

  if (*error != NULL)
    return FALSE;

  return TRUE;
}


static void
query_media_server (GomAccountMinerJob *job,
                    GError **error)
{
  GomMediaServerMiner *self = GOM_MEDIA_SERVER_MINER (job->miner);
  GomMediaServerMinerPrivate *priv = self->priv;
  gboolean dlna_supported;
  GError *local_error = NULL;
  GoaObject *object = GOA_OBJECT (job->service);
  const char *udn;

  GoaMediaServer *mediaserver = goa_object_peek_media_server (object);
  
  g_object_get (G_OBJECT (mediaserver),
                "dlna-supported", &dlna_supported,
                NULL);
  g_object_get (G_OBJECT (mediaserver),
                "udn", &udn,
                NULL);

  GList *list = NULL;
  gom_mediaserver_get_photos (priv->mngr, udn, dlna_supported, &list);

  if (list == NULL)
    g_warning ("list has no content.");

  GList *l = NULL;
  for (l = list; l != NULL; l = l->next)
    {
      account_miner_job_process_photo (job, l->data, "media_server", &local_error);
      g_slice_free (Photo, l->data);
    }
}

static GObject *
create_service (GomMiner *self,
                GoaObject *object)
{
  return g_object_ref (object);
}

static void
gom_media_server_miner_finalize (GObject *object)
{
  G_OBJECT_CLASS (gom_media_server_miner_parent_class)->finalize (object);
}

static void
gom_media_server_miner_init (GomMediaServerMiner *miner)
{
  miner->priv = G_TYPE_INSTANCE_GET_PRIVATE (miner, GOM_TYPE_MEDIA_SERVER_MINER, GomMediaServerMinerPrivate);
  miner->priv->mngr = G_OBJECT (gom_dlna_server_manager_dup_singleton ());
}

static void
gom_media_server_miner_class_init (GomMediaServerMinerClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GomMinerClass *miner_class = GOM_MINER_CLASS (klass);

  oclass->finalize = gom_media_server_miner_finalize;
  
  miner_class->goa_provider_type = "media_server";
  miner_class->miner_identifier = MINER_IDENTIFIER;
  miner_class->version = 1;

  miner_class->create_service = create_service;
  miner_class->query = query_media_server;
}
