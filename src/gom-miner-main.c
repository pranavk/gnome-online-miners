/*
 * GNOME Online Miners - crawls through your online content
 * Copyright (c) 2011 Red Hat, Inc.
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
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#ifndef INSIDE_MINER
#error "gom-miner-main.c is meant to be included, not compiled standalone"
#endif

#include <unistd.h>

#include <glib-unix.h>
#include <glib.h>

#include "tracker-ioprio.h"
#include "tracker-sched.h"

#define AUTOQUIT_TIMEOUT 5 /* seconds */

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.gnome.OnlineMiners.Miner'>"
  "    <method name='RefreshDB'>"
  "    </method>"
  "    <property name='DisplayName' type='s' access='read'/>"
  "  </interface>"
  "</node>";

static GDBusNodeInfo *introspection_data = NULL;
static GCancellable *cancellable = NULL;
static GMainLoop *loop = NULL;
static guint name_owner_id = 0;
static guint autoquit_id = 0;
static gboolean refreshing = FALSE;
static GomMiner *miner = NULL;

static gboolean
autoquit_timeout_cb (gpointer _unused)
{
  g_debug ("Timeout reached, quitting…");

  autoquit_id = 0;
  g_main_loop_quit (loop);

  return FALSE;
}

static void
ensure_autoquit_off (void)
{
  if (g_getenv (MINER_NAME "_MINER_PERSIST") != NULL)
    return;

  if (autoquit_id != 0)
    {
      g_source_remove (autoquit_id);
      autoquit_id = 0;
    }
}

static void
ensure_autoquit_on (void)
{
  if (g_getenv (MINER_NAME "_MINER_PERSIST") != NULL)
    return;

  autoquit_id =
    g_timeout_add_seconds (AUTOQUIT_TIMEOUT,
                           autoquit_timeout_cb, NULL);
}

static gboolean
signal_handler_cb (gpointer user_data)
{
  GMainLoop *loop = user_data;

  if (cancellable != NULL)
    g_cancellable_cancel (cancellable);

  g_main_loop_quit (loop);

  return FALSE;
}

static void
miner_refresh_db_ready_cb (GObject *source,
                           GAsyncResult *res,
                           gpointer user_data)
{
  GDBusMethodInvocation *invocation = user_data;
  GError *error = NULL;

  gom_miner_refresh_db_finish (GOM_MINER (source), res, &error);

  refreshing = FALSE;
  ensure_autoquit_on ();

  if (error != NULL)
    {
      g_printerr ("Failed to refresh the DB cache: %s\n", error->message);
      g_dbus_method_invocation_return_gerror (invocation, error);
    }
  else
    {
      g_dbus_method_invocation_return_value (invocation, NULL);
    }

  g_object_unref (invocation);
}

static void
handle_refresh_db (GDBusMethodInvocation *invocation)
{
  ensure_autoquit_off ();

  /* if we're refreshing already, compress with the current request */
  if (refreshing)
    return;

  refreshing = TRUE;
  cancellable = g_cancellable_new ();

  gom_miner_refresh_db_async (miner, cancellable,
                              miner_refresh_db_ready_cb, g_object_ref (invocation));
}

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  if (g_strcmp0 (method_name, "RefreshDB") == 0)
    handle_refresh_db (invocation);
  else
    g_assert_not_reached ();
}

static GVariant *
handle_get_display_name ()
{
  return g_variant_new_string (gom_miner_get_display_name (miner));
}

static GVariant *
handle_get_property (GDBusConnection       *connection,
                     const gchar           *sender,
                     const gchar           *object_path,
                     const gchar           *interface_name,
                     const gchar           *property_name,
                     GError               **error,
                     gpointer               user_data)
{
  if (g_strcmp0 (property_name, "DisplayName") == 0)
    return handle_get_display_name ();

  g_assert_not_reached ();

  return NULL;
}

static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  handle_get_property,
  NULL, /* set_property */
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
  GError *error = NULL;

  g_debug ("Connected to the session bus: %s", name);

  g_dbus_connection_register_object (connection,
                                     MINER_OBJECT_PATH,
                                     introspection_data->interfaces[0],
                                     &interface_vtable,
                                     NULL,
                                     NULL,
                                     &error);

  if (error != NULL)
    {
      g_printerr ("Error exporting object on the session bus: %s",
                  error->message);
      g_error_free (error);

      _exit (1);
    }

  miner = g_object_new (MINER_TYPE, NULL);
  g_debug ("Object exported on the session bus");
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
{
  g_debug ("Lost bus name: %s, exiting", name);

  if (cancellable != NULL)
    g_cancellable_cancel (cancellable);

  name_owner_id = 0;
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar *name,
                  gpointer user_data)
{
  g_debug ("Acquired bus name: %s", name);
}

int
main (int argc,
      char **argv)
{
  tracker_sched_idle ();
  tracker_ioprio_init ();

  errno = 0;
  if (nice (19) == -1 && errno != 0)
    {
      const gchar *str;

      str = g_strerror (errno);
      g_warning ("Couldn't set nice value to 19, %s", (str != NULL) ? str : "no error given");
    }

  ensure_autoquit_on ();
  loop = g_main_loop_new (NULL, FALSE);

  g_unix_signal_add_full (G_PRIORITY_DEFAULT,
			  SIGTERM,
			  signal_handler_cb,
			  loop, NULL);
  g_unix_signal_add_full (G_PRIORITY_DEFAULT,
			  SIGINT,
			  signal_handler_cb,
			  loop, NULL);

  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  name_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                  MINER_BUS_NAME,
                                  G_BUS_NAME_OWNER_FLAGS_NONE,
                                  on_bus_acquired,
                                  on_name_acquired,
                                  on_name_lost,
                                  NULL, NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  if (name_owner_id != 0)
    g_bus_unown_name (name_owner_id);

  return 0;
}
