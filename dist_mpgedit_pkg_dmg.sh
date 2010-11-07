#!/bin/sh
#
# Script to create a Mac OS X disk image for mpgedit
#
pmaker=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker
work_disk_image=/tmp/mpgedit_pkg_work.dmg
target_disk_image=mpgedit_pkg.dmg
tmp_disk_image=/tmp/$target_disk_image
mk_install_version=(`pwd`/mk_install_version.sh)
target_pkg_dir=(`cd macosx_installer && pwd`)
target_pkg=$target_pkg_dir/MPGEDIT.pkg
pkgroot=$target_pkg_dir/mpgedit_pkgroot

if [ `id -u -n` != "admin" ]; then
  echo "This script must be run as admin"
  exit 1
fi


. `dirname $0`/dist_parse_version.sh $0

if [ $? -ne 0 ]; then
  echo "ERROR: cd gui directory failed"
  exit 1
fi
rm -f $work_disk_image $tmp_disk_image


#
# Populate the mpgedit.app/Contents/Resources directory
#

dylib_files="\
libdecoder_mad.dylib         libmpgedit.dylib
libdecoder_mpg123.dylib      libmpgedit_decoder.dylib
libdecoder_popen.dylib"

macos_files="\
$target_pkg_dir/etc/mpgedit.c"

resource_files="\
$target_pkg_dir/etc/mpgedit.term"

resource_files2="\
$target_pkg_dir/etc/200.mpgedit_uninstall_check"

# Create Info.plist with updated version/date info from template file.
#
$mk_install_version "$target_pkg_dir/etc/Info-template.plist" \
  > "$target_pkg_dir/etc/Info.plist"
info_plist="$target_pkg_dir/etc/Info.plist"

# Create Description.plist with updated version/date info from template file.
#
$mk_install_version "$target_pkg_dir/etc/Description-template.plist" \
  > "$target_pkg_dir/etc/Description.plist"

plist_file="\
$target_pkg_dir/etc/Info.plist"

man_files="\
mpgedit.1 
xmpgedit.1
decoder.so.1
mp3decoder.sh.1
scramble_times.pl.1
unscramble.pl.1"

bin_files="\
mpgedit
mpgedit_nocurses
mp3decoder.sh"


if [ ! -d $pkgroot ]; then
mkdir -p $pkgroot/mpgedit.app/Contents/Resources
mkdir -p $pkgroot/mpgedit.app/Contents/MacOS
mkdir -p $pkgroot/mpgedit.app/Contents/MacOS/lib
mkdir -p $pkgroot/mpgedit.app/Contents/MacOS/bin
mkdir -p $pkgroot/mpgedit.app/Contents/MacOS/man/man1

cp -f $macos_files $pkgroot/mpgedit.app/Contents/MacOS
cp -f $man_files   $pkgroot/mpgedit.app/Contents/MacOS/man/man1
cp -f $bin_files   $pkgroot/mpgedit.app/Contents/MacOS/bin
cp -f $dylib_files $pkgroot/mpgedit.app/Contents/MacOS/lib
cp -f $resource_files $pkgroot/mpgedit.app/Contents/MacOS/mpgedit.term
cp -f $resource_files2 $pkgroot/mpgedit.app/Contents/Resources
cp -f $plist_file  $pkgroot/mpgedit.app/Contents

(
  cd $pkgroot/mpgedit.app/Contents/Resources
  ln -s ../MacOS/bin/mpgedit mpgedit
)

(
  cd $target_pkg_dir

  # Fixup symlink tree to be hardlink tree, as PackageMaker
  # can't deal with symlinks in the package.
  #
  if [ -h Install_Resources/License.html ]; then
    ./maketree.sh
  fi
)

(cd $pkgroot/mpgedit.app/Contents/MacOS
 cc -o mpgedit mpgedit.c
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
    -r $target_pkg_dir/Install_Resources \
    -i $target_pkg_dir/etc/Info.plist \
    -d $target_pkg_dir/etc/Description.plist
)

fi

hdiutil create -megabytes 20 $work_disk_image -layout NONE
vol=`hdid -nomount $work_disk_image`
echo "debug vol=$vol"

cat /etc/sudoadminpwd | sudo -S -u root newfs_hfs -v mpgeditinstall $vol

echo "ejecting disk $vol"
hdiutil eject $vol

echo "mounting disk '$work_disk_image'"
tmp=`hdid $work_disk_image`
target=`echo $tmp | awk '{print $2}'`
echo debug "mounted as '$target'"

echo "Copying MPGEDIT.pkg to '$target'"
cp -pr $target_pkg $target

hdiutil eject $vol

hdiutil convert -format UDZO $work_disk_image -o $tmp_disk_image

rm -f $target_disk_image
DESTDMGNAME="mpgedit_pkg_${RELEASE}.dmg"
echo DESTDMGNAME=$DESTDMGNAME

cp $tmp_disk_image $DESTDMGNAME
ls -l $DESTDMGNAME
ls -l $tmp_disk_image
rm -f $tmp_disk_image
[ -s ../../$DESTDMGNAME ] || ln -s `pwd`/$DESTDMGNAME ../..

# Following link satisfies makefile dependency
[ -s mpgedit_pkg.dmg ] || ln -s $DESTDMGNAME mpgedit_pkg.dmg
