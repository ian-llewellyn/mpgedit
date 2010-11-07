#!/bin/sh

umask 002
rm -rf Install_Resources
mkdir Install_Resources
(
cd Install_Resources
s_linktree -h ../../../../src/mpgedit/macosx_installer/Install_Resources .
rm -rf CVS
rm -f License.html
rm -f ReadMe.html
cp ../../README.sedit License.html
cp ../../NEWS.sedit   ReadMe.html
)

rm -rf Install_XMPGEDIT_Resources
mkdir Install_XMPGEDIT_Resources
(
cd Install_XMPGEDIT_Resources
s_linktree -h ../../../../src/mpgedit/macosx_installer/Install_XMPGEDIT_Resources .
rm -rf CVS
rm -f License.html
rm -f ReadMe.html
cp ../../README.sedit License.html
cp ../../NEWS.sedit   ReadMe.html
)
