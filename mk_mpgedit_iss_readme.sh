#!/bin/sh
#
# Script to format the release/date version tags for 
# the Win32 README file for the mpgedit/xmpgedit 
# ISS installer distribution.

# This one is used by inno setup
#
if [ ! -s README.txt ]; then
  ./mk_install_version.sh README_tmpl > README.txt
fi

# This one is used by dist_Linux.sh/dist_linux_rh7.sh
#
if [ ! -s README ]; then
  ./mk_install_version.sh README_tmpl > README
fi

#
# Fixup mpgedit readme versions as well
#
if [ ! -s README_win32_mpgedit.txt ]; then
  ./mk_install_version.sh README.win32_mpgedit > README_win32_mpgedit.txt
fi
if [ ! -s README_SDK_win32.txt ]; then
  ./mk_install_version.sh README_SDK.win32_tmpl > README_SDK_win32.txt
fi

#
# Fixup xmpgedit readme versions as well
#
if [ ! -s README_win32_xmpgedit.txt ]; then
  ./mk_install_version.sh README.win32_xmpgedit > README_win32_xmpgedit.txt
fi

# Fixup the versions in the .iss install files
#
if [ ! -s mpgedit_installer.iss ]; then
  ./mk_install_version.sh mpgedit_installer.iss_tmpl > mpgedit_installer.iss
fi

if [ ! -s mpgedit_sdk.iss ]; then
  ./mk_install_version.sh mpgedit_sdk.iss_tmpl > mpgedit_sdk.iss
fi

if [ ! -s gui/xmpgedit_installer.iss ]; then
  ./mk_install_version.sh gui/xmpgedit_installer.iss_tmpl > gui/xmpgedit_installer.iss
fi

if [ ! -s contrib/python/py_mpgedit/ANNOUNCEMENT.txt ]; then
  ./mk_install_version.sh contrib/python/py_mpgedit/ANNOUNCEMENT.txt_tmpl > \
      contrib/python/py_mpgedit/ANNOUNCEMENT.txt
fi
 
if [ ! -s contrib/python/py_mpgedit/LICENSE.txt ]; then
  ./mk_install_version.sh contrib/python/py_mpgedit/LICENSE.txt_tmpl > \
      contrib/python/py_mpgedit/LICENSE.txt
fi

if [ ! -s contrib/python/py_mpgedit/py_mpgedit_license.htm ]; then
  ./mk_install_version.sh \
      contrib/python/py_mpgedit/py_mpgedit_license.htm_tmpl > \
      contrib/python/py_mpgedit/py_mpgedit_license.htm
fi

if [ ! -s contrib/python/py_mpgedit/py_mpgedit.htm ]; then
  ./mk_install_version.sh contrib/python/py_mpgedit/py_mpgedit.htm_tmpl > \
      contrib/python/py_mpgedit/py_mpgedit.htm
fi

if [ ! -s README_SDK.linux ]; then
  ./mk_install_version.sh README_SDK.linux_tmpl > README_SDK.linux
fi
