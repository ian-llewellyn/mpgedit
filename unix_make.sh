#!/bin/sh
os=`uname | tr '[A-Z]' '[a-z]'`
[ $os = sunos ] && os=solaris
[ $os = hp-ux ] && os=hpux10
[ $os = darwin ] && os=macosx
[ -f Makefile ] || ln -s makefile.$os Makefile
make -f Makefile

host=`hostname | sed 's/\..*//'`
if [ $os = linux ]; then
  if [ $os = linux ]; then
    if [ \( $host = 'jake' -a -f /bin/rpm \) -o \
         \( \( $host = 'batman' -o $host = quatro \) -a \
            -f /usr/bin/rpmbuild \) ]; then
      make -f Makefile source_package
      make -f Makefile rpm_package
    fi
  fi
fi

if [ $os = macosx ]; then
  if [ \( $host = 'cobalt' -a -f /usr/bin/hdiutil \) ]; then
    make -f Makefile packages
  fi
fi

# cat /etc/sudo`id -u -n`pwd | sudo -S

