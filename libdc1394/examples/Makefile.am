MAINTAINERCLEANFILES = Makefile.in
AM_CPPFLAGS = -I $(top_srcdir)

if USE_LINUX
bin_PROGRAMS = dc1394_vloopback
endif

if XVOK
A = grab_gray_image grab_partial_image grab_color_image grab_color_image2 dc1394_multiview
else
A = grab_gray_image grab_partial_image grab_color_image grab_color_image2
endif

noinst_PROGRAMS = $(A)

if USE_LINUX
LDADD = -lraw1394
endif

grab_gray_image_SOURCES = grab_gray_image.c
grab_gray_image_LDADD = ../libdc1394/libdc1394.la

grab_color_image_SOURCES = grab_color_image.c
grab_color_image_LDADD = ../libdc1394/libdc1394.la

grab_color_image2_SOURCES = grab_color_image2.c
grab_color_image2_LDADD = ../libdc1394/libdc1394.la

grab_partial_image_SOURCES = grab_partial_image.c
grab_partial_image_LDADD = ../libdc1394/libdc1394.la

dc1394_vloopback_SOURCES = dc1394_vloopback.c affine.c affine.h
dc1394_vloopback_LDADD = -lm ../libdc1394/libdc1394.la

if XVOK
dc1394_multiview_CFLAGS = $(X_CFLAGS) $(XV_CFLAGS)
dc1394_multiview_SOURCES = dc1394_multiview.c
dc1394_multiview_LDADD = ../libdc1394/libdc1394.la $(X_LIBS) $(X_PRE_LIBS) $(XV_LIBS) -lX11 $(X_EXTRA_LIBS)
endif
