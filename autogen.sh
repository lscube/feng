#!/bin/bash

function dots ()
{
	while ps h $1 >/dev/null; do
		sleep 1;
		echo -n ".";
	done	
}

echo
echo "Bootstrapping Fenice package AutoTools configuration..."

if ! test -d config; then
	mkdir config;
fi

if which libtoolize >/dev/null 2>&1 ; then
	echo -n "Running libtoolize..."
	libtoolize --force --copy --automake &
	echo " done."
else
	echo "WARNING! Fenice SVN needs LibTool!"
	echo "Please, install it."
	echo "Aborting."
fi

if which aclocal >/dev/null 2>&1 ; then
	echo -n "Running aclocal..."
	aclocal -I config &
	dots $! 
	echo " done."
fi

if which autoheader >/dev/null 2>&1 ; then
	echo -n "Running autoheader..."
	autoheader 2>&1 | grep -v AC_TRY_RUN &
	dots $! 
	echo " done."
fi
if which autoconf >/dev/null 2>&1 ; then
	echo -n "Running autoconf..."
	autoconf 2>&1 | grep -v AC_TRY_RUN &
	dots $! 
	echo " done."
fi
if which automake >/dev/null 2>&1 ; then
	echo -n "Running automake..."
	automake --gnu --add-missing --copy --force-missing 2>&1 | grep -v installing&
	dots $! 
	echo " done."
fi
#echo -n "Copying missing files..."
#	cp -f admin/lt* libltdl
#	cp -f admin/config.* libltdl
#	cp -f admin/install-sh libltdl
#	cp -f admin/mkinstalldirs .
#	cp -f libltdl/acinclude.m4 .
#echo " done."

echo "All done. Bye."
echo
