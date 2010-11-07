#!/bin/sh
#
# Script to create a Mac OS X disk image for xmpgedit
#
if [ `id -u -n` != "admin" ]; then
  echo "This script must be run as admin"
  exit 1
fi

# dist_parse_version.sh sets "RELEASE" from version.h
. `dirname $0`/dist_parse_version.sh $0

mk_install_version=(`pwd`/mk_install_version.sh)
target_pkg_dir=(`cd macosx_installer && pwd`)

cd gui
rm -f /tmp/xmpgedit_work.dmg /tmp/xmpgedit.dmg

#
# Populate the xmpgedit.app/Contents/Resources directory
#
xpm_files="\
blankdigit_led.xpm      colon_led.xpm           five_led.xpm
one_led.xpm             play.xpm                seven_led.xpm
two_led.xpm             blankpunct_led.xpm      colon_minute_led.xpm
four_led.xpm            pause.xpm               record.xpm
six_led.xpm             volume1.xpm             close.xpm
eight_led.xpm           next_t.xpm              period_led.xpm
record_green.xpm        stop.xpm                zero_led.xpm
colon_hour_led.xpm      eject.xpm               nine_led.xpm
period_second_led.xpm   record_red.xpm          three_led.xpm"

dylib_files="\
../libdecoder_mad.dylib         ../libmpgedit.dylib
../libdecoder_mpg123.dylib      ../libmpgedit_decoder.dylib
../libdecoder_popen.dylib"

macos_files="\
xmpgedit.app/Contents/MacOS/xmpgedit.c
xmpgedit.app/Contents/MacOS/xmpgedit.sh"

info_plist="$target_pkg_dir/etc_xmpgedit/Info.plist"
if [ ! -f "$info_plist" ]; then
  # Create Info.plist with updated version/date info from template file.
  #
  $mk_install_version "$target_pkg_dir/etc_xmpgedit/Info-template.plist" \
    > "$target_pkg_dir/etc_xmpgedit/Info.plist"
fi

mkdir -p /tmp/xmpgedit.app/Contents/Resources
mkdir -p /tmp/xmpgedit.app/Contents/MacOS
cp -f $xpm_files   /tmp/xmpgedit.app/Contents/Resources
cp -f $dylib_files /tmp/xmpgedit.app/Contents/Resources
cp -f $macos_files /tmp/xmpgedit.app/Contents/MacOS
cp -f $info_plist  /tmp/xmpgedit.app/Contents
cp -f xmpgedit     /tmp/xmpgedit.app/Contents/Resources

(cd /tmp/xmpgedit.app/Contents/MacOS
 make xmpgedit
)

hdiutil create -megabytes 20 /tmp/xmpgedit_work.dmg -layout NONE
vol=`hdid -nomount /tmp/xmpgedit_work.dmg`
echo "debug vol=$vol"

cat /etc/sudoadminpwd | sudo -S -u root newfs_hfs -v xmpgeditinstall $vol

echo "ejecting disk $vol"
hdiutil eject $vol

echo "mounting disk '/tmp/xmpgedit_work.dmg'"
tmp=`hdid /tmp/xmpgedit_work.dmg`
target=`echo $tmp | awk '{print $2}'`
echo debug "mounted as '$target'"

echo "Copying xmpgedit.app to '$target'"
cp -pr /tmp/xmpgedit.app $target

hdiutil eject $vol

hdiutil convert -format UDZO /tmp/xmpgedit_work.dmg -o /tmp/xmpgedit.dmg

cd ..
rm -f xmpgedit.dmg
DESTDMGNAME="xmpgedit_${RELEASE}.dmg"
cp /tmp/xmpgedit.dmg $DESTDMGNAME
(cd /tmp && rm -rf /tmp/xmpgedit.app)
ls -l $DESTDMGNAME
ls -l /tmp/xmpgedit.dmg
rm -f /tmp/xmpgedit.dmg
[ -s ../../$DESTDMGNAME ] || ln -s `pwd`/$DESTDMGNAME ../..

# Following link satisfies makefile dependency
[ -s xmpgedit.dmg ] || ln -s $DESTDMGNAME xmpgedit.dmg
