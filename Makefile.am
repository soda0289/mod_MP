AUTOMAKE_OPTIONS = foreign dist-bzip2

EXTRA_DIST = m4/apache.m4 \
	     m4/apache_test.m4 \
	     m4/config_nice.m4 \
             LICENSE \
	     README

ACLOCAL_AMFLAGS = "-Im4/"

clean: 
	rm -rf Makefile.in
	rm -rf config.in

SUBDIRS = src
