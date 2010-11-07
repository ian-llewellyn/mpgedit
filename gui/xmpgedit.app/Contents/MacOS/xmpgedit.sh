#!/bin/sh
#debug=1
export PATH=/usr/X11R6/bin:$PATH
path=$0
if [ -n "`echo $path | grep '^/'`" ]; then
  dir=`dirname $path`
else
  dir=`pwd`
fi
resdir=$dir/../Resources
etcdir=$dir/etc

[ -n "$debug" ] && echo $HOME > /tmp/xmpgedit.out
[ -n "$debug" ] && echo $PATH >> /tmp/xmpgedit.out
[ -n "$debug" ] && echo etcdir=$etcdir >> /tmp/xmpgedit.out
dyld_library_path=$resdir:$dir/lib
[ -n "$debug" ] && echo DYLD_LIBRARY_PATH=$dyld_library_path HOME=$HOME _GTK_PIXMAP_PATH=$resdir >> /tmp/xmpgedit.out

# Display variable for xmpgedit, but not for X server or window manager.
#
env_display=0
NO_X_SERVER=0
if [ -n "$DISPLAY" ]; then 
  env_display=1
  display=$DISPLAY
else
  # Search for unused X server lock, or first lock owned by this user.
  #
  display=0
  lockname="/tmp/.X${display}-lock"
  while [ -f $lockname -a ! -O $lockname ]; do
    display=`expr $display + 1`
    lockname="/tmp/.X${display}-lock"
  done
  display=":$display"
  
  if [ ! -f $lockname ]; then
    NO_X_SERVER=1
  else
    #
    # Test for X server running...
    #
    NO_X_SERVER=0
    XPID=`cat $lockname`
    kill -0 $XPID > /dev/null 2>&1
    if [ $? -ne 0 ]; then
      NO_X_SERVER=1
    fi
  fi
[ -n "$debug" ] && echo debug NO_X_SERVER=$NO_X_SERVER >> /tmp/xmpgedit.out
fi

if [ $env_display -eq 1 ]; then
  env -i DISPLAY=$display DYLD_LIBRARY_PATH=$dyld_library_path HOME=$HOME _GTK_PIXMAP_PATH=$resdir $resdir/xmpgedit "$@"
  exit $?
elif [ $NO_X_SERVER -eq 1 ]; then
[ -n "$debug" ] && echo "Starting X server display=$display" >> /tmp/xmpgedit.out
  X $display  > /dev/null 2>&1 &

  cnt=0
  [ -n "$debug" ] && xdpyinfo -display $display >> /tmp/xmpgedit.out
  [ -z "$debug" ] && xdpyinfo -display $display 2>&1
  sts=$?
  while [ $sts -ne 0 -a $cnt -lt 60 ]; do
    [ -n "$debug" ] && echo "Waiting for X server to start (display=$display)..." >> /tmp/xmpgedit.out
    sleep 1
    [ -n "$debug" ] && xdpyinfo -display $display >> /tmp/xmpgedit.out
    [ -z "$debug" ] && xdpyinfo -display $display 2>&1
    sts=$?
    cnt=`expr $cnt + 1`
  done
  if [ $cnt -eq 60 ]; then
    [ -n "$debug" ] && echo "debug X server never started!" >> /tmp/xmpgedit.out
    exit 1
  fi

  # Start window manager, since we just start X server, there certainly is not
  # one already running.
  #
  [ -n "$debug" ] && echo "Starting quartz-wm..." >> /tmp/xmpgedit.out
  env DISPLAY=$display quartz-wm > /dev/null 2>&1 &
else
  #
  # Test for running window manager, and start one if one is not found.
  #
  cnt1=`ps auxw | grep 'quartz-wm$' | grep -c -v grep`
  cnt2=`ps auxw | grep 'twm$'       | grep -c -v grep`

  [ -n "$debug" ] && echo debug cnt1=$cnt1 cnt2=$cnt2 >> /tmp/xmpgedit.out
  if [ $cnt1 -eq 0 -a $cnt2 -eq 0 ]; then
    env DISPLAY=$display quartz-wm > /dev/null 2>&1 &
  fi
fi

if [ -n "`echo x$1 | grep 'x-psn'`" ]; then
  shift
fi
env -i DISPLAY=$display DYLD_LIBRARY_PATH=$dyld_library_path HOME=$HOME _GTK_PIXMAP_PATH=$resdir $resdir/xmpgedit "$@"
status=$?
[ -n "$debug" ] && echo "After xmpgedit $status" >> /tmp/xmpgedit.out
exit $status
