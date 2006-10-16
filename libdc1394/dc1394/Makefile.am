MAINTAINERCLEANFILES = Makefile.in
lib_LTLIBRARIES = libdc1394.la

SUBDIRS = $(dc1394platform) vendor
DIST_SUBDIRS = linux macosx

libdc1394_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@

libdc1394_la_SOURCES =  \
	control.c 	\
	control.h	\
	offsets.h	\
	format7.c        \
	register.c       \
	register.h       \
	utils.c          \
	utils.h          \
	internal.c       \
	internal.h       \
	conversions.c    \
	conversions.h    \
	bayer.c          \
	kernel-video1394.h

libdc1394_la_LIBADD = $(dc1394platform)/libdc1394-$(dc1394platform).la

# headers to be installed
pkginclude_HEADERS = \
	control.h \
	utils.h \
	conversions.h \
	register.h
