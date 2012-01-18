/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* unit-test-keyrings.c: Test basic keyring functionality

   Copyright (C) 2007 Stefan Walter

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Stef Walter <stef@memberwebs.com>
*/

#include "config.h"

#include "mate-keyring.h"
#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static GList* keyrings = NULL;

#define PASSWORD "my-keyring-password"
#define KEYRING_NAME "unit-test-keyring"
#define INVALID_KEYRING_NAME "invalid-keyring-name"
#define DISPLAY_NAME "Item Display Name"
#define SECRET "item-secret"

static GMainLoop *mainloop = NULL;

static gboolean
quit_loop (gpointer data)
{
	g_main_loop_quit (data);
	return TRUE;
}

static void
mainloop_run (int timeout)
{
	guint id = 0;

	if (timeout)
		id = g_timeout_add (timeout, quit_loop, mainloop);
	g_main_loop_run (mainloop);
	if (timeout)
		g_source_remove (id);
}

static void
test_remove_incomplete (void)
{
	MateKeyringResult res;

	res = mate_keyring_delete_sync (KEYRING_NAME);
	if (res != MATE_KEYRING_RESULT_NO_SUCH_KEYRING)
		g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static void
test_create_keyring (void)
{
	MateKeyringResult res;

	res = mate_keyring_create_sync (KEYRING_NAME, PASSWORD);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static void
test_create_keyring_already_exists (void)
{
	MateKeyringResult res;

	mate_keyring_create_sync (KEYRING_NAME, PASSWORD);
	res = mate_keyring_create_sync (KEYRING_NAME, PASSWORD);
	g_assert_cmpint (MATE_KEYRING_RESULT_ALREADY_EXISTS, ==, res);
}

static void
test_set_default_keyring (void)
{
	MateKeyringResult res;
	gchar* name;

	res = mate_keyring_set_default_keyring_sync (KEYRING_NAME);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	res = mate_keyring_set_default_keyring_sync (INVALID_KEYRING_NAME);
	g_assert_cmpint (MATE_KEYRING_RESULT_NO_SUCH_KEYRING, ==, res);

	res = mate_keyring_get_default_keyring_sync (&name);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	g_assert (name != NULL);
	g_assert_cmpstr (name, ==, KEYRING_NAME);
}

static void
test_delete_keyring (void)
{
	MateKeyringResult res;
	gchar* name;

	res = mate_keyring_delete_sync (KEYRING_NAME);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	res = mate_keyring_delete_sync (KEYRING_NAME);
	g_assert_cmpint (MATE_KEYRING_RESULT_NO_SUCH_KEYRING, ==, res);

	res = mate_keyring_get_default_keyring_sync (&name);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	/* "returning deleted keyring as default" */
	g_assert(name == NULL || strcmp (name, KEYRING_NAME) != 0);
}

static void
test_recreate_keyring (void)
{
	MateKeyringResult res;

	/* Create the test keyring again and set as default */
	res = mate_keyring_create_sync (KEYRING_NAME, PASSWORD);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	res = mate_keyring_set_default_keyring_sync (KEYRING_NAME);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static gint
compare_ids (gconstpointer a, gconstpointer b)
{
	if (a < b)
		return -1;
	return a == b ? 0 : 1;
}

static void
test_create_list_items (void)
{
	MateKeyringResult res;
	guint id, id2, id3;
	GList *ids;
	MateKeyringItemInfo *info;
	MateKeyringAttributeList* attrs;

	/* Try in an invalid keyring */
	res = mate_keyring_item_create_sync (INVALID_KEYRING_NAME, MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      DISPLAY_NAME, NULL, SECRET, FALSE, &id);
	g_assert_cmpint (MATE_KEYRING_RESULT_NO_SUCH_KEYRING, ==, res);

	/* Create for real in valid keyring */
	res = mate_keyring_item_create_sync (KEYRING_NAME, MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      DISPLAY_NAME, NULL, SECRET, FALSE, &id);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* Update the item, shouldn't create new */
	res = mate_keyring_item_create_sync (KEYRING_NAME, MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      DISPLAY_NAME, NULL, SECRET, TRUE, &id3);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	/* "Updated item doesn't have the same id" */
	g_assert_cmpint (id, ==, id3);

	/* Update in NULL keyring, should use default */
	res = mate_keyring_item_create_sync (NULL, MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      DISPLAY_NAME, NULL, SECRET, TRUE, &id3);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	/* "Updated item doesn't have the same id" */
	g_assert_cmpint (id, ==, id3);

	/* Create new,  shouldn't update */
	res = mate_keyring_item_create_sync (KEYRING_NAME, MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      "Another display name", NULL, SECRET, FALSE, &id2);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	/* "Two items created with the same id" */
	g_assert_cmpint (id, !=, id2);

	/* Set some attributes, NULL keyring = default */
	attrs = mate_keyring_attribute_list_new ();
	mate_keyring_attribute_list_append_string (attrs, "bender", "rocks");
	res = mate_keyring_item_set_attributes_sync (NULL, id, attrs);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* List ids that were created */
	res = mate_keyring_list_item_ids_sync (KEYRING_NAME, &ids);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* Check that they're the same ids */
	/* "Wrong number of ids created" */
	g_assert_cmpint (g_list_length (ids), ==, 2);
	if (g_list_length (ids) == 2) {
		ids = g_list_sort (ids, compare_ids);
		g_assert_cmpint (id, ==, GPOINTER_TO_UINT (ids->data));
		g_assert_cmpint (id2, ==, GPOINTER_TO_UINT (ids->next->data));
	}

	/* Now make sure both have that same secret */
	res = mate_keyring_item_get_info_sync (KEYRING_NAME, id, &info);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	if (res == MATE_KEYRING_RESULT_OK)
		/* "Secret has changed" */
		g_assert_cmpstr (mate_keyring_item_info_get_secret (info), ==, SECRET);

	/* And try it with a NULL (ie: default) keyring */
	res = mate_keyring_item_get_info_sync (NULL, id2, &info);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	if (res == MATE_KEYRING_RESULT_OK)
		g_assert_cmpstr (mate_keyring_item_info_get_secret (info), ==, SECRET);

	/* Set the info back, should work */
	res = mate_keyring_item_set_info_sync (NULL, id2, info);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* Make sure it's still the same */
	res = mate_keyring_item_get_info_sync (KEYRING_NAME, id, &info);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	if (res == MATE_KEYRING_RESULT_OK)
		g_assert_cmpstr (mate_keyring_item_info_get_secret (info), ==, SECRET);

	/* Now delete the item */
	res = mate_keyring_item_delete_sync (NULL, id);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static void
test_find_keyrings (void)
{
	MateKeyringResult res;
	MateKeyringAttributeList* attrs;
	MateKeyringAttribute *attr;
	MateKeyringFound* f;
	guint id, i;
	GList *found;
	guint num;

	/* Unique for every run */
	num  = time (NULL);

	attrs = mate_keyring_attribute_list_new ();
	mate_keyring_attribute_list_append_string (attrs, "dog", "woof");
	mate_keyring_attribute_list_append_string (attrs, "bird", "cheep");
	mate_keyring_attribute_list_append_string (attrs, "iguana", "");
	mate_keyring_attribute_list_append_uint32 (attrs, "num", num);

	/* Create the item */
	res = mate_keyring_item_create_sync ("session", MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      "Barnyard", attrs, SECRET, TRUE, &id);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* Now try to find it */
	res = mate_keyring_find_items_sync (MATE_KEYRING_ITEM_GENERIC_SECRET, attrs, &found);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	/* "Too many items found" */
	g_assert_cmpint (g_list_length (found), ==, 1);

	f = (MateKeyringFound*)found->data;
	/* "Wrong item found" */
	g_assert (f->item_id == id);
	/* "Found in wrong keyring" */
	g_assert_cmpstr (f->keyring, ==, "session");
	/* "Wrong secret came back" */
	g_assert_cmpstr (f->secret, ==, SECRET);

	res = mate_keyring_item_get_attributes_sync ("session", id, &attrs);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* Make sure that dog does in fact woof */
	attr = NULL;
	for(i = 0; i < attrs->len; ++i)
	{
		attr = &mate_keyring_attribute_list_index (attrs, i);
		if (strcmp (attr->name, "dog") == 0)
			break;
	}

	g_assert (attr != NULL);
	if (attr) {
		/* "invalid attribute found" */
		g_assert_cmpstr (attr->name, ==, "dog");
		/* "invalid attribute type" */
		g_assert_cmpint (attr->type, ==, MATE_KEYRING_ATTRIBUTE_TYPE_STRING);
		/* "invalid attribute value" */
		g_assert_cmpstr (attr->value.string, ==, "woof");
	}
}

/*
 * A find that does not match should return 'Not Found':
 * http://bugzilla.gnome.org/show_bug.cgi?id=476682
 */
static void
test_find_invalid (void)
{
	MateKeyringResult res;
	MateKeyringAttributeList* attrs;
	GList *found;

	attrs = mate_keyring_attribute_list_new ();
	mate_keyring_attribute_list_append_string (attrs, "fry-unset-attribute", "rocks");

	/* Now try to find it */
	res = mate_keyring_find_items_sync (MATE_KEYRING_ITEM_GENERIC_SECRET, attrs, &found);
	g_assert_cmpint (MATE_KEYRING_RESULT_NO_MATCH, ==, res);
}

static void
test_lock_keyrings (void)
{
	MateKeyringResult res;

	res = mate_keyring_lock_sync (KEYRING_NAME);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	res = mate_keyring_unlock_sync (KEYRING_NAME, PASSWORD);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* An unlock when already unlocked is fine */
	res = mate_keyring_unlock_sync (KEYRING_NAME, PASSWORD);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	res = mate_keyring_unlock_sync ("boooyaaah", PASSWORD);
	g_assert_cmpint (MATE_KEYRING_RESULT_NO_SUCH_KEYRING, ==, res);
}

static void
test_change_password (void)
{
	MateKeyringResult res;

	res = mate_keyring_change_password_sync (KEYRING_NAME, PASSWORD, "new password");
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static void
test_keyring_info (void)
{
	MateKeyringResult res;
	MateKeyringInfo *info;

	res = mate_keyring_get_info_sync (NULL, &info);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	res = mate_keyring_set_info_sync (NULL, info);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static void
test_list_keyrings (void)
{
	MateKeyringResult res;
	GList *l;

	res = mate_keyring_list_keyring_names_sync (&keyrings);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	printf("\t\tkeyrings:\n");
	for (l = keyrings; l; l = g_list_next (l))
		printf("\t\t  %s\n", (gchar*)l->data);
}

static MateKeyringResult grant_access_result = MATE_KEYRING_RESULT_CANCELLED;

static void
done_grant_access (MateKeyringResult res, gpointer data)
{
	grant_access_result = res;
	g_main_loop_quit (mainloop);
}

static void
test_keyring_grant_access (void)
{
	GList *acl;
	MateKeyringResult res;
	gpointer op;
	guint id;

	/* Create teh item */
	res = mate_keyring_item_create_sync (NULL, MATE_KEYRING_ITEM_GENERIC_SECRET,
	                                      "Barnyard", NULL, SECRET, FALSE, &id);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* Grant strange program access (async) */
	grant_access_result = MATE_KEYRING_RESULT_CANCELLED;
	op = mate_keyring_item_grant_access_rights (NULL, "Strange Application",
	                                             "/usr/bin/strangeness", id,
	                                             MATE_KEYRING_ACCESS_READ,
	                                             done_grant_access, NULL, NULL);
	/* "return null op" */
	g_assert (op != NULL);
	/* "callback already called" */
	g_assert_cmpint (grant_access_result, ==, MATE_KEYRING_RESULT_CANCELLED);

	mainloop_run (2000);

	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, grant_access_result);

	/* Now list the stuff */
	res = mate_keyring_item_get_acl_sync (NULL, id, &acl);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	mate_keyring_acl_free (acl);
}

/* -----------------------------------------------------------------------------
 * SIMPLE PASSWORD API
 */

static MateKeyringPasswordSchema our_schema = {
	MATE_KEYRING_ITEM_GENERIC_SECRET,
	{
		{ "dog", MATE_KEYRING_ATTRIBUTE_TYPE_STRING },
		{ "legs", MATE_KEYRING_ATTRIBUTE_TYPE_UINT32 },
		{ NULL, 0 }
	},
};

static void
done_store_password (MateKeyringResult res, gpointer data)
{
	*((MateKeyringResult*)data) = res;
	g_main_loop_quit (mainloop);
}

static void
test_store_password (void)
{
	MateKeyringResult res;
	gpointer op;

	/* Synchronous, bad arguments */
	res = mate_keyring_store_password_sync (&our_schema, NULL,
	                                         "Display name", "password",
	                                         NULL);
	g_assert_cmpint (MATE_KEYRING_RESULT_BAD_ARGUMENTS, ==, res);

	/* Synchronous, save to default keyring */
	res = mate_keyring_store_password_sync (&our_schema, NULL,
	                                         "Display name", "password",
	                                         "dog", "woof",
	                                         "legs", 4,
	                                         NULL);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);

	/* Asynchronous, save to session */
	res = MATE_KEYRING_RESULT_CANCELLED;
	op = mate_keyring_store_password (&our_schema, MATE_KEYRING_SESSION,
	                                   "Display name", "password",
	                                   done_store_password, &res, NULL,
	                                   "dog", "woof",
	                                   "legs", 4,
	                                   NULL);
	/* "async operation is NULL" */
	g_assert (op != NULL);
	/* "callback already called" */
	g_assert_cmpint (res, ==, MATE_KEYRING_RESULT_CANCELLED);

	mainloop_run (2000);

	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static MateKeyringResult find_password_result;

static void
done_find_password (MateKeyringResult res, const gchar* password, gpointer unused)
{
	find_password_result = res;

	if(res == MATE_KEYRING_RESULT_OK) {
		/* "Null password returned" */
		g_assert (password != NULL);
		/* "Wrong returned from find" */
		g_assert_cmpstr (password, ==, "password");
	}

	g_main_loop_quit (mainloop);
}

static void
test_find_password (void)
{
	MateKeyringResult res;
	gchar *password;
	gpointer op;

	/* Synchronous, bad arguments */
	res = mate_keyring_find_password_sync (&our_schema, &password,
	                                        NULL);
	g_assert_cmpint (MATE_KEYRING_RESULT_BAD_ARGUMENTS, ==, res);

	/* Synchronous, valid*/
	res = mate_keyring_find_password_sync (&our_schema, &password,
	                                        "dog", "woof",
	                                        "legs", 4,
	                                        NULL);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
	/* "Null password returned" */
	g_assert (password != NULL);
	/* "Wrong returned from find" */
	g_assert_cmpstr (password, ==, "password");
	mate_keyring_free_password (password);

	/* Asynchronous, less arguments */
	find_password_result = MATE_KEYRING_RESULT_CANCELLED;
	op = mate_keyring_find_password (&our_schema,
	                                  done_find_password, NULL, NULL,
	                                  "legs", 4,
	                                  NULL);
	/* "async operation is NULL" */
	g_assert (op != NULL);
	/* "callback already called" */
	g_assert (find_password_result == MATE_KEYRING_RESULT_CANCELLED);

	mainloop_run (2000);

	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, find_password_result);
}

static MateKeyringResult find_no_password_result;

static void
done_find_no_password (MateKeyringResult res, const gchar* password, gpointer unused)
{
	find_no_password_result = res;
	g_assert (password == NULL);
	g_main_loop_quit (mainloop);
}

static void
test_find_no_password (void)
{
	MateKeyringResult res;
	gchar *password = NULL;
	gpointer op;

	/* Synchronous, valid*/
	res = mate_keyring_find_password_sync (&our_schema, &password,
	                                        "dog", "grunt",
	                                        "legs", 1000,
	                                        NULL);
	g_assert_cmpint (MATE_KEYRING_RESULT_NO_MATCH, ==, res);
	g_assert (password == NULL);

	/* Asynchronous, less arguments */
	find_no_password_result = MATE_KEYRING_RESULT_CANCELLED;
	op = mate_keyring_find_password (&our_schema,
	                                  done_find_no_password, NULL, NULL,
	                                  "dog", "grunt",
	                                  "legs", 1000,
	                                  NULL);
	g_assert (op != NULL);
	g_assert (find_no_password_result == MATE_KEYRING_RESULT_CANCELLED);

	mainloop_run (2000);

	g_assert_cmpint (MATE_KEYRING_RESULT_NO_MATCH, ==, find_no_password_result);
}

static void
done_delete_password (MateKeyringResult res, gpointer data)
{
	*((MateKeyringResult*)data) = res;
	g_main_loop_quit (mainloop);
}

static void
test_delete_password (void)
{
	MateKeyringResult res;
	gpointer op;

	/* Synchronous, bad arguments */
	res = mate_keyring_delete_password_sync (&our_schema, NULL);
	g_assert_cmpint (MATE_KEYRING_RESULT_BAD_ARGUMENTS, ==, res);

	/* Synchronous, no match */
	res = mate_keyring_delete_password_sync (&our_schema,
	                                          "dog", "waoof",
	                                          "legs", 5,
	                                          NULL);
	g_assert_cmpint (MATE_KEYRING_RESULT_NO_MATCH, ==, res);

	/* Asynchronous, less arguments */
	res = MATE_KEYRING_RESULT_CANCELLED;
	op = mate_keyring_delete_password (&our_schema,
	                                    done_delete_password, &res, NULL,
	                                    "legs", 4,
	                                    NULL);
	/* "async operation is NULL" */
	g_assert (op != NULL);
	/* "callback already called" */
	g_assert (res == MATE_KEYRING_RESULT_CANCELLED);

	mainloop_run (2000);

	/* Should have already been deleted by the second call above */
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static void
test_cleanup (void)
{
	MateKeyringResult res;

	res = mate_keyring_delete_sync (KEYRING_NAME);
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static void
test_set_display (void)
{
	MateKeyringResult res;

	/* Deprecated method */
	res = mate_keyring_daemon_set_display_sync (":0.0");
	g_assert_cmpint (MATE_KEYRING_RESULT_DENIED, ==, res);
}

static void
test_setup_environment (void)
{
	MateKeyringResult res;

	res = mate_keyring_daemon_prepare_environment_sync ();
	g_assert_cmpint (MATE_KEYRING_RESULT_OK, ==, res);
}

static pid_t daemon_pid = 0;

static gboolean
daemon_start (void)
{
	GError *err = NULL;
	gchar *args[5];
	const gchar *path, *service, *address;
	gchar *output = NULL;
	gint exit_status = 1;
	GError *error = NULL;

	/* Need to have DBUS running */
	address = g_getenv ("DBUS_SESSION_BUS_ADDRESS");
	if (!address || !address[0]) {
		g_printerr ("\nNo DBUS session available, skipping tests!\n\n");
		return FALSE;
	}

	/* Check if mate-keyring-daemon has testing enabled */
	if (!g_spawn_command_line_sync (MATE_KEYRING_DAEMON_PATH " --version",
	                                &output, NULL, &exit_status, &error)) {
		g_printerr ("Couldn't launch '%s': %s", MATE_KEYRING_DAEMON_PATH,
		            error->message);
		return FALSE;
	}

	/* For some reason --version failed */
	if (exit_status != 0 || strstr (output, "testing: enabled") == NULL) {
		g_printerr ("Skipping tests since no testing enabled mate-keyring-daemin is available\n");
		g_free (output);
		return FALSE;
	}

	g_free (output);

	path = g_getenv ("MATE_KEYRING_TEST_PATH");
	if (path && !path[0])
		path = NULL;

	service = g_getenv ("MATE_KEYRING_TEST_SERVICE");
	if (service && !service[0])
		service = NULL;

	if (!path && !service) {
		g_mkdir_with_parents ("/tmp/keyring-test/data", 0700);
		g_setenv ("MATE_KEYRING_TEST_PATH", "/tmp/keyring-test/data", TRUE);
		g_setenv ("MATE_KEYRING_TEST_SERVICE", "org.mate.keyring.Test", TRUE);

		g_printerr ("Starting mate-keyring-daemon...\n");

		args[0] = MATE_KEYRING_DAEMON_PATH;
		args[1] = "--foreground";
		args[2] = "--control-directory";
		args[3] = "/tmp/keyring-test";
		args[4] = NULL;

		if (!g_spawn_async (NULL, args, NULL, G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_DO_NOT_REAP_CHILD,
							NULL, NULL, &daemon_pid, &err)) {
			g_error ("couldn't start mate-keyring-daemon for testing: %s",
					 err && err->message ? err->message : "");
			g_assert_not_reached ();
		}

		/* Let it startup properly */
		sleep (2);
	}

	return TRUE;
}

static void
daemon_stop (void)
{
	if (daemon_pid)
		kill (daemon_pid, SIGTERM);
	daemon_pid = 0;
}

int
main (int argc, char **argv)
{
	int ret = 0;

	g_test_init (&argc, &argv, NULL);

	mainloop = g_main_loop_new (NULL, FALSE);

	g_test_add_func ("/keyrings/remove-incomplete", test_remove_incomplete);
	g_test_add_func ("/keyrings/create-keyring", test_create_keyring);
	g_test_add_func ("/keyrings/create-keyring-already-exists", test_create_keyring_already_exists);
	g_test_add_func ("/keyrings/set-default-keyring", test_set_default_keyring);
	g_test_add_func ("/keyrings/delete-keyring", test_delete_keyring);
	g_test_add_func ("/keyrings/recreate-keyring", test_recreate_keyring);
	g_test_add_func ("/keyrings/create-list-items", test_create_list_items);
	g_test_add_func ("/keyrings/find-keyrings", test_find_keyrings);
	g_test_add_func ("/keyrings/find-invalid", test_find_invalid);
	g_test_add_func ("/keyrings/lock-keyrings", test_lock_keyrings);
	g_test_add_func ("/keyrings/change-password", test_change_password);
	g_test_add_func ("/keyrings/keyring-info", test_keyring_info);
	g_test_add_func ("/keyrings/list-keyrings", test_list_keyrings);
	g_test_add_func ("/keyrings/keyring-grant-access", test_keyring_grant_access);
	g_test_add_func ("/keyrings/store-password", test_store_password);
	g_test_add_func ("/keyrings/find-password", test_find_password);
	g_test_add_func ("/keyrings/find-no-password", test_find_no_password);
	g_test_add_func ("/keyrings/delete-password", test_delete_password);
	g_test_add_func ("/keyrings/cleanup", test_cleanup);
	g_test_add_func ("/keyrings/set-display", test_set_display);
	g_test_add_func ("/keyrings/setup-environment", test_setup_environment);

	if (daemon_start ()) {
		ret = g_test_run ();
		daemon_stop ();
	}

	return ret;
}
