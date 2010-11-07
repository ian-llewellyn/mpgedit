#!/bin/sh
#
# Script to create a Mac OS X disk image for xmpgedit
#
pmaker=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker
work_disk_image=/tmp/xmpgedit_gtk_pkg_work.dmg
target_disk_image=xmpgedit_gtk_pkg.dmg
tmp_disk_image=/tmp/$target_disk_image
target_pkg_dir=(`cd macosx_installer && pwd`)
mk_install_version=(`pwd`/mk_install_version.sh)
target_pkg=$target_pkg_dir/XMPGEDIT_GTK.pkg
pkgroot=$target_pkg_dir/xmpgedit_pkgroot

if [ `id -u -n` != "admin" ]; then
  echo "This script must be run as admin"
  exit 1
fi


. `dirname $0`/dist_parse_version.sh $0

cd gui
if [ $? -ne 0 ]; then
  echo "ERROR: cd gui directory failed"
  exit 1
fi

# Eject any mounted xmpgedit install images...
#
if [ `mount | grep -c /Volumes/xmpgeditinstall` -gt 0 ]; then
  mount_point=`mount | grep /Volumes/xmpgeditinstall | sed 's| .*||'`
  for i in $mount_point; do
    hdiutil eject $i
  done
fi

rm -f $work_disk_image $tmp_disk_image

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
xmpgedit.app/Contents/MacOS/xmpgedit_gtk.c
xmpgedit.app/Contents/MacOS/xmpgedit_gtk.sh"

lib_files="\
./libgtk-x11-2.0.0.400.9.dylib
./libgtk-x11-2.0.dylib
./libgtk-x11-2.0.0.dylib
./libgdk-x11-2.0.0.400.9.dylib
./libgdk-x11-2.0.0.dylib
./libgdk-x11-2.0.dylib
./libatk-1.0.0.600.1.dylib
./libatk-1.0.0.dylib
./libatk-1.0.dylib
./libgdk_pixbuf-2.0.0.400.9.dylib
./libgdk_pixbuf-2.0.0.dylib
./libgdk_pixbuf-2.0.dylib
./libpangoxft-1.0.0.600.0.dylib
./libpangoxft-1.0.0.dylib
./libpangoxft-1.0.dylib
./libpangox-1.0.0.600.0.dylib
./libpangox-1.0.0.dylib
./libpangox-1.0.dylib
./libpango-1.0.0.600.0.dylib
./libpango-1.0.0.dylib
./libpango-1.0.dylib
./libgobject-2.0.0.400.6.dylib
./libgobject-2.0.0.dylib
./libgobject-2.0.dylib
./libgmodule-2.0.0.400.6.dylib
./libgmodule-2.0.0.dylib
./libgmodule-2.0.dylib
./libglib-2.0.0.400.6.dylib
./libglib-2.0.0.dylib
./libglib-2.0.dylib
./libintl.3.4.3.dylib
./libintl.3.dylib
./libintl.dylib
./libiconv.2.3.0.dylib
./libiconv.2.dylib
./libiconv.dylib
./libintl.1.0.1.dylib
./libintl.1.dylib
./libpangoft2-1.0.0.600.0.dylib
./libpangoft2-1.0.0.dylib
./libpangoft2-1.0.dylib
./gtk-2.0
./gtk-2.0/2.4.0
./gtk-2.0/2.4.0/loaders
./gtk-2.0/2.4.0/loaders/libpixbufloader-ani.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-xbm.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-wbmp.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-tiff.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-tga.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-ras.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-pnm.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-png.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-pcx.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-jpeg.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-ico.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-gif.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-bmp.so
./gtk-2.0/2.4.0/loaders/libpixbufloader-xpm.so
pango/1.4.0/modules/pango-hangul-fc.so
pango/1.4.0/modules/pango-indic-fc.so
pango/1.4.0/modules/pango-hebrew-fc.so
pango/1.4.0/modules/pango-thai-fc.so
pango/1.4.0/modules/pango-basic-x.so
pango/1.4.0/modules/pango-basic-fc.so
pango/1.4.0/modules/pango-arabic-fc.so"

etc_files="\
pangorc
pango.modules
pangox.aliases
gdk-pixbuf.loaders"

# Create Info.plist with updated version/date info from template file.
#
$mk_install_version "$target_pkg_dir/etc_xmpgedit/Info-template.plist" \
  > "$target_pkg_dir/etc_xmpgedit/Info.plist"
info_plist="$target_pkg_dir/etc_xmpgedit/Info.plist"

# Create Description.plist with updated version/date info from template file.
#
$mk_install_version "$target_pkg_dir/etc_xmpgedit/Description-template.plist" \
  > "$target_pkg_dir/etc_xmpgedit/Description.plist"

if [ ! -d $pkgroot ]; then
mkdir -p $pkgroot/xmpgedit.app/Contents/Resources
mkdir -p $pkgroot/xmpgedit.app/Contents/MacOS
mkdir -p $pkgroot/xmpgedit.app/Contents/MacOS/etc
mkdir -p $pkgroot/xmpgedit.app/Contents/MacOS/lib

cp -f $xpm_files   $pkgroot/xmpgedit.app/Contents/Resources
cp -f $dylib_files $pkgroot/xmpgedit.app/Contents/Resources
cp -f $macos_files $pkgroot/xmpgedit.app/Contents/MacOS
cp -f $info_plist  $pkgroot/xmpgedit.app/Contents
cp -f xmpgedit     $pkgroot/xmpgedit.app/Contents/Resources
(cd xmpgedit.app/Contents/MacOS/lib && find $lib_files | cpio -pd $pkgroot/xmpgedit.app/Contents/MacOS/lib)
(cd ../../../src/mpgedit/gui/xmpgedit.app/Contents/MacOS/etc && find $etc_files | cpio -pd $pkgroot/xmpgedit.app/Contents/MacOS/etc)

(cd $pkgroot/xmpgedit.app/Contents/MacOS
 cc -o xmpgedit xmpgedit_gtk.c
)

rm -rf $target_pkg
fi

if [ ! -s $target_pkg ]; then
#
# Build package installer...
#
(
    cd $target_pkg_dir
    $pmaker -build \
    -p $target_pkg \
    -f $pkgroot \
    -r $target_pkg_dir/Install_XMPGEDIT_Resources \
    -i $target_pkg_dir/etc_xmpgedit/Info.plist \
    -d $target_pkg_dir/etc_xmpgedit/Description.plist
)

fi

hdiutil create -megabytes 60 $work_disk_image -layout NONE
vol=`hdid -nomount $work_disk_image`
echo "debug vol=$vol"

cat /etc/sudoadminpwd | sudo -S -u root newfs_hfs -v xmpgeditinstall $vol

echo "ejecting disk $vol"
hdiutil eject $vol

echo "mounting disk '$work_disk_image'"
tmp=`hdid $work_disk_image`
target=`echo $tmp | awk '{print $2}'`
echo debug "mounted as '$target'"

echo "Copying XMPGEDIT_GTK.pkg to '$target'"
cp -pr $target_pkg $target

hdiutil eject $vol

hdiutil convert -format UDZO $work_disk_image -o $tmp_disk_image

cd ..
rm -f $target_disk_image
DESTDMGNAME="xmpgedit_gtk_pkg_${RELEASE}.dmg"

cp $tmp_disk_image $DESTDMGNAME
#(cd /tmp && rm -rf $pkgroot/xmpgedit.app)
ls -l $DESTDMGNAME
ls -l $tmp_disk_image
rm -f $tmp_disk_image
[ -s ../../$DESTDMGNAME ] || ln -s `pwd`/$DESTDMGNAME ../..

# Following link satisfies makefile dependency
[ -s $target_disk_image ] || ln -s $DESTDMGNAME $target_disk_image
