/*
 * GNOME Online Miners - crawls through your online content
 * Copyright (c) 2014 Pranav Kant
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

#include "gom-media-server-miner.h"
#include "gom-dlna-servers-manager.h"
#include "gom-dlna-server.h"
#include "gom-upnp-media-container2.h"

#define MINER_IDENTIFIER "gd:media-server:miner:a4a47a3e-eb55-11e3-b983-14feb59cfa0e"

struct _GomMediaServerMinerPrivate {
  GObject *mngr;
};

typedef struct {
  const gchar *name;
  const gchar *url;
  const gchar *mimetype;
  const gchar *path;
  const gchar *type;
} PhotoItem;

G_DEFINE_TYPE_WITH_PRIVATE (GomMediaServerMiner, gom_media_server_miner, GOM_TYPE_MINER)


static gboolean
account_miner_job_process_photo (GomAccountMinerJob *job,
                                 PhotoItem *photo,
                                 const gchar *creator,
                                 GError **error)
{
  const gchar *photo_id;
  const gchar *photo_name;
  const gchar *mimetype;
  const gchar *photo_link;
  gchar *identifier;
  const gchar *class = "nmm:Photo";
  gchar *resource = NULL;
  gboolean resource_exists;
  gchar *contact_resource;
  gchar **tmp_arr;

  tmp_arr = g_strsplit_set (photo->path, "/", -1);

  photo_id = g_strdup (tmp_arr[g_strv_length (tmp_arr) - 1]);
  photo_link = photo->url;
  photo_name = photo->name;

  mimetype = photo->mimetype;

  identifier = g_strdup_printf ("media-server:%s", photo_id);

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
  g_strfreev (tmp_arr);

  if (*error != NULL)
    return FALSE;

  return TRUE;
}


static PhotoItem *
generate_photo_item_from_variant (GVariant *var)
{
  GVariant *var1 = NULL;
  gchar *str = NULL;
  PhotoItem *photo;

  photo = g_slice_new0 (PhotoItem);

  g_variant_lookup (var, "DisplayName", "s", &str);
  photo->name = g_strdup (str);

  g_variant_lookup (var, "MIMEType", "s", &str);
  photo->mimetype = g_strdup (str);

  g_variant_lookup (var, "Path", "o", &str);
  photo->path = g_strdup (str);

  g_variant_lookup (var, "Type", "s", &str);
  photo->type = g_strdup (str);

  if (g_str_equal (photo->type, "container"))
    {
      photo->url = NULL;
      goto out;
    }

  g_variant_lookup (var, "URLs", "@as", &var1);
  g_variant_get_child (var1, 0, "s", &str);
  photo->url = g_strdup (str);

 out:
  return photo;
}

/* Processes the variant, puts photos items in photos_list and return
   a list of containers to be processed again. */
static GList *
process_variant (GVariant *out, GList **photos_list)
{
  GVariantIter *iter = NULL;
  GVariant *var = NULL;
  GList *containers = NULL;
  PhotoItem *photo;
  gchar *path = NULL;

  g_variant_get (out, "aa{sv}", &iter);
  while (g_variant_iter_loop (iter, "@a{sv}", &var))
    {
      photo = generate_photo_item_from_variant (var);
      if (g_str_equal (photo->type, "image.photo"))
        {
          *photos_list = g_list_prepend (*photos_list, photo);
        }
      else if (g_str_equal (photo->type, "container"))
        {
          path = g_strdup (photo->path);
          containers = g_list_prepend (containers, path);

          g_slice_free (PhotoItem, photo);
        }
    }

  return containers;
}


/*
  Recursively find photo items in the directory structure.
  Keep inserting photo items into Glist **photo_list as they
  are found.
*/
static void
find_photos (const gchar   *obj_path,
             GList        **photos_list)
{
  UpnpMediaContainer2 *proxy = NULL;
  const gchar *const filter[] = {"DisplayName","Type","Path", "URLs", "MIMEType"};
  GVariant *var = NULL;
  GError *error = NULL;
  GList *containers = NULL, *tmp;

  proxy = upnp_media_container2_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        "com.intel.dleyna-server",
                                                        obj_path,
                                                        NULL, /* GCancellable */
                                                        &error);

  if (error != NULL)
    {
      g_warning ("Unable to get proxy for Upnp.MediaContainer2 : %s",
                 error->message);
      g_error_free (error);
      goto out;
    }

  upnp_media_container2_call_list_children_sync (proxy,
                                                 0,
                                                 0,
                                                 filter,
                                                 &var,
                                                 NULL, /* GCancellable */
                                                 &error);

  if (error != NULL)
    {
      g_warning ("Unable to call ListChildren : %s",
                 error->message);
      g_error_free (error);
      goto out;
    }
    
  if (var == NULL)
    goto out;

  containers = process_variant (var, photos_list);
  if (containers == NULL)
    goto out;

  for (tmp = containers; tmp != NULL; tmp = tmp->next)
    {
      find_photos (tmp->data, photos_list);
    }
  g_list_free_full (containers, g_free);

 out:
  g_variant_unref (var);
  g_object_unref (proxy);
}


static GList *
gom_media_server_get_photos (GObject     *mngr,
                             const gchar *udn)
{
  GomDlnaServersManager *dlna_mngr = GOM_DLNA_SERVERS_MANAGER (mngr);
  GomDlnaServer *server = gom_dlna_servers_manager_get_server (dlna_mngr, udn);
  GList *photos_list = NULL;
  GError *error = NULL;
  PhotoItem *photo;
  GVariant *out, *var;
  GVariantIter *iter = NULL;
  const gchar *obj_path;

  if (server == NULL)
    return NULL; /* Server is offline. */

  if (gom_dlna_server_get_searchable (server))
    {
      out = gom_dlna_server_search_objects (server, &error);
      if (error != NULL)
        {
          g_warning ("Unable to search objects on server : %s",
                     error->message);
          g_error_free (error);
          return NULL;
        }

      g_variant_get (out, "aa{sv}", &iter);
      while (g_variant_iter_loop (iter, "@a{sv}", &var))
        {
          photo = generate_photo_item_from_variant (var);
          photos_list = g_list_prepend (photos_list, photo);
        }

      g_variant_iter_free (iter);
    }
  else
    {
      obj_path = gom_dlna_server_get_object_path (server);
      /* This calls ListChildren recursively. */
      find_photos (obj_path, &photos_list);
    }

  return photos_list;
}

static void
query_media_server (GomAccountMinerJob *job,
                    GError **error)
{
  GomMediaServerMiner *self = GOM_MEDIA_SERVER_MINER (job->miner);
  GomMediaServerMinerPrivate *priv = self->priv;
  const gchar *udn, *friendly_name;
  GError *local_error = NULL;
  GoaMediaServer *mediaserver;
  GList *photos_list, *tmp;
  GomDlnaServer *server;
  GoaObject *object = GOA_OBJECT (g_hash_table_lookup (job->services, "photos"));

  mediaserver   = goa_object_peek_media_server (object);
  udn           = goa_media_server_get_udn (mediaserver);
  server        = gom_dlna_servers_manager_get_server (GOM_DLNA_SERVERS_MANAGER (priv->mngr),
                                                       udn);
  friendly_name = gom_dlna_server_get_friendly_name (server);
  photos_list   = gom_media_server_get_photos (priv->mngr, udn);
  for (tmp = photos_list; tmp != NULL; tmp = tmp->next)
    {
      account_miner_job_process_photo (job, tmp->data, friendly_name, &local_error);
      g_slice_free (PhotoItem, tmp->data);
      if (local_error != NULL)
        {
          g_warning ("Unable to process photo : %s",
                     local_error->message);
          g_error_free (local_error);
        }
    }
  g_object_unref (server);
}

static GHashTable *
create_services (GomMiner *self,
                 GoaObject *object)
{
  GHashTable *services;

  services = g_hash_table_new_full (g_str_hash, g_str_equal,
                                    NULL, (GDestroyNotify) g_object_unref);

  if (gom_miner_supports_type (self, "photos"))
    g_hash_table_insert (services, "photos", g_object_ref (object));

  return services;
}

static void
gom_media_server_miner_init (GomMediaServerMiner *miner)
{
  GomMediaServerMinerPrivate *priv;

  priv = miner->priv = gom_media_server_miner_get_instance_private (miner);
  priv->mngr = G_OBJECT (gom_dlna_servers_manager_dup_singleton ());
}

static void
gom_media_server_miner_class_init (GomMediaServerMinerClass *klass)
{
  GomMinerClass *miner_class = GOM_MINER_CLASS (klass);

  miner_class->goa_provider_type = "media_server";
  miner_class->miner_identifier = MINER_IDENTIFIER;
  miner_class->version = 1;

  miner_class->create_services = create_services;
  miner_class->query = query_media_server;
}
