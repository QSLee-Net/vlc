nvdecdir = $(pluginsdir)/nvdec
nvdec_LTLIBRARIES =

libnvdec_plugin_la_SOURCES = \
	hw/nvdec/nvdec.c hw/nvdec/nvdec_fmt.h hw/nvdec/nvdec_priv.h \
	hw/nvdec/hw_pool.c hw/nvdec/hw_pool.h
libnvdec_plugin_la_CFLAGS = $(AM_CFLAGS) $(FFNVCODEC_CFLAGS)
libnvdec_plugin_la_LIBADD = $(LIBDL) libvlc_hxxxhelper.la
if HAVE_NVDEC
nvdec_LTLIBRARIES += libnvdec_plugin.la
endif

libnvdec_chroma_plugin_la_SOURCES = hw/nvdec/chroma.c \
	hw/nvdec/nvdec_fmt.h hw/nvdec/nvdec_priv.h
libnvdec_chroma_plugin_la_CFLAGS = $(AM_CFLAGS) $(FFNVCODEC_CFLAGS)
if HAVE_NVDEC
nvdec_LTLIBRARIES += libnvdec_chroma_plugin.la
endif

libglinterop_nvdec_plugin_la_SOURCES = hw/nvdec/nvdec_gl.c \
	video_output/opengl/interop.h hw/nvdec/nvdec_fmt.h hw/nvdec/nvdec_priv.h
libglinterop_nvdec_plugin_la_CFLAGS = $(AM_CFLAGS) $(FFNVCODEC_CFLAGS)
libglinterop_nvdec_plugin_la_LIBADD = $(LIBDL)
if HAVE_GL
if HAVE_NVDEC
nvdec_LTLIBRARIES += libglinterop_nvdec_plugin.la
endif
endif
