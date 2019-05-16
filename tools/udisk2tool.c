#include <udisks/udisks.h>

static UDisksClient *client = NULL;
static GMainLoop *loop = NULL;
static gchar *opt_info_object = NULL;
static gchar *opt_info_device = NULL;
static gchar *opt_info_drive = NULL;

static UDisksObject *lookup_object_by_drive (const gchar *drive)
{
  UDisksObject *ret;
  GList *objects;
  GList *l;
  gchar *full_drive_object_path;

  ret = NULL;

  full_drive_object_path = g_strdup_printf ("/org/freedesktop/UDisks2/drives/%s", drive);

  objects = g_dbus_object_manager_get_objects (udisks_client_get_object_manager (client));
  for (l = objects; l != NULL; l = l->next)
    {
      UDisksObject *object = UDISKS_OBJECT (l->data);
      UDisksDrive *drive_iface;

      if (g_strcmp0 (g_dbus_object_get_object_path (G_DBUS_OBJECT (object)), full_drive_object_path) != 0)
        continue;

      drive_iface = udisks_object_peek_drive (object);
      if (drive_iface != NULL)
        {
          ret = g_object_ref (object);
          goto out;
        }
    }

 out:
  g_list_foreach (objects, (GFunc) g_object_unref, NULL);
  g_list_free (objects);
  g_free (full_drive_object_path);

  return ret;
}

static gchar * variant_to_string_with(GVariant *value)
{
  gchar *value_str;

  if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
    {
      value_str = g_variant_dup_string (value, NULL);
    }
  else if (g_variant_is_of_type (value, G_VARIANT_TYPE_BYTESTRING))
    {
      value_str = g_variant_dup_bytestring (value, NULL);
    }
  else
    {
      value_str = NULL;
    }
  return value_str;
}

static void print_interface_properties (GDBusProxy *proxy)
{
    gchar **cached_properties;
    guint n;

    cached_properties = g_dbus_proxy_get_cached_property_names (proxy);
    for (n = 0; cached_properties != NULL && cached_properties[n] != NULL; n++)
    {
        const gchar *property_name = cached_properties[n];
        GVariant *value;
        gchar *value_str;

        value = g_dbus_proxy_get_cached_property (proxy, property_name);
        value_str = variant_to_string_with(value);
        g_print("prop:%s value:%s\n", property_name,value_str?value_str:"none");
        if (value_str != NULL)
            g_free (value_str);
        g_variant_unref (value);
    }
    g_strfreev (cached_properties);
}

static void print_object (UDisksObject *object)
{
    GList *interface_proxies;
    GList *l;

    interface_proxies = g_dbus_object_get_interfaces (G_DBUS_OBJECT (object));
    for (l = interface_proxies; l != NULL; l = l->next)
    {
        GDBusProxy *iproxy = G_DBUS_PROXY (l->data);
        g_print("%s\n", g_dbus_proxy_get_interface_name (iproxy));
        print_interface_properties (iproxy);
    }
    g_list_foreach (interface_proxies, (GFunc) g_object_unref, NULL);
    g_list_free (interface_proxies);
}

int main (int argc, char **argv)
{
    GList *objects;
    GList *l;
    const gchar *object_path;
    GError *error = NULL;

    loop = NULL;
    loop = g_main_loop_new (NULL, FALSE);
    client = udisks_client_new_sync (NULL, /* GCancellable */ &error);

    objects = g_dbus_object_manager_get_objects (udisks_client_get_object_manager (client));
    for (l = objects; l != NULL; l = l->next)
    {
        UDisksObject *object = UDISKS_OBJECT (l->data);

        object_path = g_dbus_object_get_object_path (G_DBUS_OBJECT (object));
        g_print ("%s \n", object_path);
        print_object(object);
        g_print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    }
    g_list_foreach (objects, (GFunc) g_object_unref, NULL);
    g_list_free (objects);
    goto out;
/*
    object = lookup_object_by_drive (opt_info_drive);
    if (object == NULL)
    {
        g_printerr ("Error looking up object for drive %s\n", opt_info_drive);
        goto out;
    }
*/
out:
    return 0;
}

