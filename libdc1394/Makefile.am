# process this file with automake to create a Makefile.in

SUBDIRS = src

install-exec-hook:
	@if [ ! -c /dev/raw1394 ]; then                                 \
		echo;                                                   \
		echo "***********************************************"; \
		echo "Please install libraw1394 before installing"; \
		echo "the 1394-Based Digital Camera Control Library"; \
		echo "***********************************************"; \
		echo;                                                   \
	fi
