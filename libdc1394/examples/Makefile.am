MAINTAINERCLEANFILES = Makefile.in
bin_PROGRAMS = dc1394_vloopback
noinst_PROGRAMS = grab_gray_image grab_partial_image dc1394_multiview grab_color_image

grab_gray_image_SOURCES = grab_gray_image.c
grab_gray_image_LDADD = ../libdc1394/libdc1394_control.la -lraw1394

grab_color_image_SOURCES = grab_color_image.c
grab_color_image_LDADD = ../libdc1394/libdc1394_control.la -lraw1394

grab_partial_image_SOURCES = grab_partial_image.c
grab_partial_image_LDADD = ../libdc1394/libdc1394_control.la -lraw1394

dc1394_multiview_SOURCES = dc1394_multiview.c
dc1394_multiview_LDADD = ../libdc1394/libdc1394_control.la -lraw1394 -L/usr/X11R6/lib -lXv -lXext -lX11 

dc1394_vloopback_SOURCES = dc1394_vloopback.c
dc1394_vloopback_LDADD = ../libdc1394/libdc1394_control.la -lraw1394
