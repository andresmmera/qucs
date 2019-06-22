## Process this file with automake to produce Makefile.in
#
# Makefile.am
#
# Automake input file.
#
# Copyright (C) 2015 Felix Salfelder
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this package; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
# Boston, MA 02110-1301, USA.
#

install:

if COND_WIN32
	RMDIR /S /Q qucs-test
	git clone https://github.com/Qucs/qucs-test/
	RMDIR /S /Q ${pkgdatadir}/examples
	md ${pkgdatadir}/examples
	move /Y qucs-test ${pkgdatadir}/examples
	RMDIR /S /Q qucs-test
else
	rm -rf ./qucs-test	
	git clone https://github.com/Qucs/qucs-test/
	rm -rf ${pkgdatadir}/examples
	mkdir ${pkgdatadir}/examples
	mv -f ./qucs-test/testsuite/* ${pkgdatadir}/examples
	rm -rf qucs-test
endif
