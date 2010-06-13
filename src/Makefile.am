plugin_LTLIBRARIES = libgstchromium.la libgstburn.la libgstdodge.la \
   libgstexclusion.la

# sources used to compile Chromium
libgstchromium_la_SOURCES = gstchromium.c gstchromium.h
# compiler and linker flags used to compile Chromium, set in configure.ac
libgstchromium_la_CFLAGS = $(GST_CFLAGS)
libgstchromium_la_LIBADD = $(GST_LIBS)
libgstchromium_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstchromium_la_LIBTOOLFLAGS = --tag=disable-static

# sources used to compile this Burn
libgstburn_la_SOURCES = gstburn.c gstburn.h
# compiler and linker flags used to Burn, set in configure.ac
libgstburn_la_CFLAGS = $(GST_CFLAGS)
libgstburn_la_LIBADD = $(GST_LIBS)
libgstburn_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstburn_la_LIBTOOLFLAGS = --tag=disable-static

# sources used to compile Dodge
libgstdodge_la_SOURCES = gstdodge.c gstdodge.h
# compiler and linker flags used to compile Dodge, set in configure.ac
libgstdodge_la_CFLAGS = $(GST_CFLAGS)
libgstdodge_la_LIBADD = $(GST_LIBS)
libgstdodge_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstdodge_la_LIBTOOLFLAGS = --tag=disable-static

# sources used to compile Dodge
libgstexclusion_la_SOURCES = gstexclusion.c gstexclusion.h
# compiler and linker flags used to compile Dodge, set in configure.ac
libgstexclusion_la_CFLAGS = $(GST_CFLAGS)
libgstexclusion_la_LIBADD = $(GST_LIBS)
libgstexclusion_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstexclusion_la_LIBTOOLFLAGS = --tag=disable-static


# headers we need but don't want installed
noinst_HEADERS = gstplugin.h
