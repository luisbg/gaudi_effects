plugin_LTLIBRARIES = libgstchromium.la libgstburn.la

# sources used to compile this plug-in
libgstchromium_la_SOURCES = gstchromium.c gstchromium.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstchromium_la_CFLAGS = $(GST_CFLAGS)
libgstchromium_la_LIBADD = $(GST_LIBS)
libgstchromium_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstchromium_la_LIBTOOLFLAGS = --tag=disable-static

# sources used to compile this plug-in
libgstburn_la_SOURCES = gstburn.c gstburn.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstburn_la_CFLAGS = $(GST_CFLAGS)
libgstburn_la_LIBADD = $(GST_LIBS)
libgstburn_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstburn_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstplugin.h
