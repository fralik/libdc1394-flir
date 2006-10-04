# the libdc1394_control itself
MAINTAINERCLEANFILES = Makefile.in
lib_LTLIBRARIES = libdc1394.la

libdc1394_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@

libdc1394_la_SOURCES =  \
	dc1394_control.c 	\
	dc1394_control.h	\
	dc1394_offsets.h	\
	dc1394_format7.c        \
	dc1394_vendor_avt.c 	\
	dc1394_vendor_avt.h	\
	dc1394_vendor_pixelink.c\
	dc1394_vendor_pixelink.h\
	dc1394_register.c       \
	dc1394_register.h       \
	dc1394_utils.c          \
	dc1394_utils.h          \
	dc1394_internal.c       \
	dc1394_internal.h       \
	dc1394_conversions.c    \
	dc1394_conversions.h    \
	dc1394_bayer.c          \
	kernel-video1394.h


if USE_LINUX
libdc1394_la_SOURCES += \
	dc1394_capture.c \
	dc1394_capture_linux.h \
	dc1394_control_linux.c \
	dc1394_topology.h \
	dc1394_topology.c \
        raw1394support.h \
	dc1394_linux.h
endif

if USE_MACOSX
libdc1394_la_SOURCES += \
	dc1394_capture_macosx.c \
	dc1394_capture_macosx.h \
	dc1394_control_macosx.c \
	dc1394_macosx.h
endif

# headers to be installed
pkginclude_HEADERS = dc1394_control.h dc1394_utils.h dc1394_vendor_avt.h dc1394_conversions.h dc1394_register.h dc1394_vendor_pixelink.h 
if USE_MACOSX
pkginclude_HEADERS += dc1394_capture_macosx.h
endif
