#!/bin/sh
product=mpgedit_0.7p2
install_root=/usr/local
modinstall="`dirname $0`/.install.sh"
if [ -f "$modinstall" ]; then
  if [ `basename $0` = install.sh ]; then
    exec $modinstall $@
    exit 0
  fi
fi

if [ -n "$MPGEDIT_VERSION" ]; then
  product="$MPGEDIT_VERSION"
fi

man1="mpgedit.1 xmpgedit.1 decoder.so.1 mp3decoder.sh.1
      scramble_times.pl.1 scramble.pl.1 unscramble.pl.1"
lib="libdecoder_mpg123.so libdecoder_mad.so libdecoder_popen.so libmpglib_mpgedit.so"
bin="mpgedit mpgedit_nocurses mp3decoder.sh"
guibin="xmpgedit"
share="eject.xpm pause.xpm record.xpm next_t.xpm play.xpm stop.xpm 
       record_green.xpm record_red.xpm close.xpm volume1.xpm
       blankdigit_led.xpm eight_led.xpm nine_led.xpm  seven_led.xpm two_led.xpm
       blankpunct_led.xpm five_led.xpm  one_led.xpm   six_led.xpm   zero_led.xpm
       colon_led.xpm      colon_hour_led.xpm colon_minute_led.xpm
       four_led.xpm  period_led.xpm period_second_led.xpm three_led.xpm"

Y_FLAG=NO
H_FLAG=NO
U_FLAG=NO
F_FLAG=YES
M_FLAG=NO


# Configure manpath
#
do_man_config()
{
  man_root=$1
 
  if [ $os = "darwin" ]; then
    echo 'Calling makewhatis, this may take some time...'
    echo makewhatis $man_root
    makewhatis $man_root
    echo 'done.'
    unset man_root
    return
  fi

  if [ ! -f "/etc/man.config" ]; then
    unset man_root
    return
  fi

  if [ $Y_FLAG = "NO" ]; then
    echo
    echo "'$man_root' not found in /etc/man.config"
    echo "This is needed for man to find mpgedit man pages"
    echo
    echo -n "  Do you want to add this directory path now? [Y/n] "
    read line
    line=`echo $line | tr '[A-Z]' '[a-z]'`
  else
    line=$Y_FLAG
  fi
  if [ "$line" != "n" -a "$line" != "no" ]; then
    if [ -f "/etc/man.config" ]; then
      count=`grep "^MANPATH.*" "$man_root" | wc -l`
      if [ $count -eq 0 ]; then
        echo "MANPATH	$man_root" >> "/etc/man.config"
        echo 'Calling makewhatis, this may take some time...'
        echo makewhatis $man_root
        makewhatis $man_root
        echo 'done.'
      fi
    fi
  fi
  unset man_root count line
}



# Attempt "college best" to install pixmap_path in gtkrc file
#
do_gtkrc_config()
{
  if [ ! -d "/usr/local/etc/gtk-2.0" ]; then
    if [ "`uname`" = "Linux" ]; then
      count=`locate etc/gtk-2.0 | grep 'gtk-2.0$' | wc -l`
      if [ $count -gt 1 ]; then
        echo "WARNING: Unable to determine GTK+ 2.0 install directory"
        echo "         You must manually configure the gtkrc file pixmap_path"
        return
      fi
      gtkrc="`locate etc/gtk-2.0 | grep 'gtk-2.0$'`"
    fi
  elif [ "$os" = "darwin" ]; then
    gtkrc="/sw/share/themes/Default/gtk-2.0/gtkrc"
  else
    gtkrc="/usr/local/etc/gtk-2.0"
  fi
  if [ -f "$gtkrc/gtkrc" ]; then
    tmptst=`grep pixmap_path "$gtkrc/gtkrc"`
    if [ -z "$tmptst" ]; then
      # This is simple, add pixmap_path to existing gtkrc file
      #
      echo "pixmap_path \"$install_root/share/xmpgedit\"" >> "$gtkrc/gtkrc"
    else
      # Determine if xmpgedit is already in pixmap path
      #
      tmptst=`grep "$install_root/share/xmpgedit" "$gtkrc/gtkrc"`
      if [ -z "$tmptst" ]; then
        sed -e "s|pixmap_path \(.*\)\"|pixmap_path \1:$install_root/share/xmpgedit\"|" "$gtkrc/gtkrc" > /tmp/gtkrc$$
        i=1
        while [ -f "$gtkrc/gtkrc.$i" ]; do
          i=`expr $i + 1`
        done
        cp "$gtkrc/gtkrc" "$gtkrc/gtkrc.$i"
        cp "/tmp/gtkrc$$" "$gtkrc/gtkrc"
      fi
    fi
  else
    # This is simple, create gtkrc with needed entry
    #
    echo "pixmap_path \"$install_root/share/xmpgedit\"" > "$gtkrc/gtkrc"
  fi

  unset count tmptst gtkrc i
}


do_install()
{
  get_install_root
  
  echo "Installing '$product' in '$install_root'"
  install -d $man_base/man1
  install -d $install_root/lib
  install -d $install_root/bin
  
  echo "Installing man pages ($man_base/man1)..."
  install -m 444 $man1 $man_base/man1
  
  echo "Installing shared libraries ($install_root/lib)..."
  install $lib $install_root/lib
  
  echo "Installing executables ($install_root/bin)..."
  install $bin $install_root/bin

  if [ -d "gui" ]; then
    cd gui
  fi
  echo "Installing GUI executables ($install_root/bin)..."
  install $guibin $install_root/bin

  echo "Installing xmpgedit pixmaps ($install_root/share/xmpgedit)..."
  #
  # Note: trailing slash here is extremely important, as it tells -D
  # xmpgedit is a directory, not a file.
  #
  install -d        $install_root/share/xmpgedit/
  mkdir   -p        $install_root/share/xmpgedit/
  install $share    $install_root/share/xmpgedit/
  #
  # Attempt to install pixmaps in a standard location.
  #
  if [ -d "/usr/share/pixmaps" -a ! -d "/usr/share/pixmaps/xmpgedit" ]; then
    ln -s $install_root/share/xmpgedit /usr/share/pixmaps/xmpgedit
  elif [ -d "/usr/local/share" -a ! -d "/usr/local/share/xmpgedit" ]; then
    ln -s $install_root/share/xmpgedit /usr/local/share/xmpgedit
  fi

  if [ -d "gui" ]; then
    cd ..
  fi
  
  rm -f $install_root/lib/libmpgedit_decoder.$shlibext
  ln -s libdecoder_mpg123.$shlibext \
        $install_root/lib/libmpgedit_decoder.$shlibext

  if [ $F_FLAG = "YES" ]; then
  ( cd /usr/lib
    [ -f "libmpgedit_decoder.$shlibext" ] || ln -s $install_root/lib/libmpgedit_decoder.$shlibext .
  )
  fi
  
  if [ -f "/etc/ld.so.conf" ]; then
    line=`grep $install_root/lib /etc/ld.so.conf`
    if [ -z "$line" ]; then
  
      # Short-circuit interactive question if -y provided on command line
      #
      if [ $Y_FLAG = "NO" ]; then
        echo
        echo "$install_root/lib not found in /etc/ld.so.conf"
        echo "This is needed for proper operation of mpgedit"
        echo
        echo -n "  Do you want to add this now? [Y/n] "
        read line
        line=`echo $line | tr '[A-Z]' '[a-z]'`
      else
        line=$Y_FLAG
      fi
      if [ "$line" != "n" -a "$line" != "no" ]; then
        echo "updating /etc/ld.so.conf..."
        echo "$install_root/lib" >> /etc/ld.so.conf
      fi
    fi
     # Always run ldconfig, even if install path was previously in ld.so.conf,
     # because this install may have added new libraries not previously in
     # the library cache.
    /sbin/ldconfig
  fi

  if [ $F_FLAG = "YES" ]; then
    do_man_config $man_base
    do_gtkrc_config
  fi

  echo "Finished installing '$product'"
  echo
  echo "Remember to add '$install_root/bin' to your PATH environment variable"
  echo "Remember to add '$install_root/man/man1' to your MANPATH environment variable"
}


do_uninstall()
{
  get_install_root "un"
  if [ ! -d "$install_root" ]; then
    echo "ERROR: uninstall directory does not exist"
    exit 1
  fi

  echo "Uninstalling '$product' in '$install_root'"
  for i in $man1; do
    if [ -f $man_base/man1/$i ]; then
      echo "rm -f $man_base/man1/$i"
            rm -f $man_base/man1/$i
    fi
  done
  for i in $lib; do
    if [ -f $install_root/lib/$i ]; then
      echo "rm -f $install_root/lib/$i"
            rm -f $install_root/lib/$i
    fi
  done
  for i in $bin $guibin; do
    if [ -f $install_root/bin/$i ]; then
     echo "rm -f $install_root/bin/$i"
           rm -f $install_root/bin/$i
    fi
  done
  if [ -d $install_root/share/xmpgedit ]; then
    #
    # These are really symbolic links to the directories if they exist.
    # Don't use rm -rf on these, because if they really  are 
    # directories, you may really not mean to delete them.
    #
    [ -L /usr/share/pixmaps/xmpgedit ] && rm -f "/usr/share/pixmaps/xmpgedit"
    [ -L /usr/local/share/xmpgedit ] && rm -f "/usr/local/share/xmpgedit"
    for i in $share; do
      if [ -f $install_root/share/xmpgedit/$i ]; then
       echo "rm -f $install_root/share/xmpgedit/$i"
             rm -f $install_root/share/xmpgedit/$i
      fi
    done
    rmdir $install_root/share/xmpgedit/
  fi
  if [ -h $install_root/lib/libmpgedit_decoder.$shlibext ]; then
    echo "rm -f $install_root/lib/libmpgedit_decoder.$shlibext"
          rm -f "$install_root/lib/libmpgedit_decoder.$shlibext"
          rm -f "/usr/lib/libmpgedit_decoder.$shlibext"
  fi
  echo "Finished uninstalling '$product' in '$install_root'"
}


get_install_root()
{
  # Path already specified on command line
  #
  if [ -n "$install_root" ]; then
    return
  fi

  prefix=$1
  echo "Current ${prefix}install location is '$install_root'"
  echo -n "  Change location? [y/N] "
  read line
  line=`echo $line | tr '[A-Z]' '[a-z]'`
  if [ "$line" = "y" -o "$line" = "yes" ]; then
    ok=no
    echo
    while [ "$ok" = "no" ]; do
      echo "Enter ${prefix}install directory"
      read new_install_root
      echo "New ${prefix}install directory: '$new_install_root'"
      echo -n "  Is this correct? "
      read line
      line=`echo $line | tr '[A-Z]' '[a-z]'`
      if [ "$line" = "y" -o "$line" = "yes" ]; then
        ok="yes"
      fi
    done
    install_root=$new_install_root
  fi
}


do_usage()
{
  echo \
"usage: install.sh [-u][-y][-F][-m manroot][-h][path]
       -u: uninstall product
       -y: default answer YES to all interactive questions
       -F: suppress installation functions not needed by Fink
       -m: specify the man page install root directory
       -h: display this help
       path: directory to install/uinstall product"
       
}


#
# ============================ main =======================
#
if [ `id -u` -ne 0 ]; then
  echo "ERROR: Can only install '$product' as root"
  exit 1;
fi

PATH=$PATH:/usr/sbin:/usr/bin
export PATH

os=`uname | tr '[A-Z]' '[a-z]'`
if [ "$os" = "darwin" ]; then
  install_root=/opt/local
  shlibext="dylib"
  PATH=$PATH:/usr/libexec
  export PATH
elif [ "$os" = "hp-ux" ]; then
  shlibext="sl"
else
  shlibext="so"
fi

# Fix up the shared library file extension for the current OS flavor
lib=`echo $lib | sed "s|\.so|.$shlibext|g"`

while [ `echo "x$1" | grep -c 'x-'` = "1" ]; do
  t=`echo "x$1" | grep '^x-u$'`
  if [ "x$1" = "x-u" ]; then
    U_FLAG=YES
    un="un"
  elif [ "x$1" = "x-y" ]; then
    Y_FLAG=YES
  elif [ "x$1" = "x-h" ]; then
    H_FLAG=YES
  elif [ "x$1" = "x-F" ]; then
    F_FLAG="NO"
  elif [ "x$1" = "x-m" ]; then
    M_FLAG="YES"
    shift
    if [ -z "$1" ]; then
      echo "ERROR: -m requires an argument"
      exit 1
    fi
    man_base="$1"
  else
    echo "unrecognized option '$1'"
    exit 1
  fi
  shift
done

if [ -n "$1" ]; then
  if [ ! -d "$1" ]; then
    echo "ERROR: directory does not exist '$1'"
    do_usage
    exit 1
  fi
  install_root="$1"
  shift
fi

if [ $H_FLAG = "YES" ]; then
  do_usage
  exit 0
fi

if [ $M_FLAG = "NO" ]; then
  man_base="$install_root/man"
fi

if [ $U_FLAG = "YES" ]; then
  do_uninstall
else
  do_install
  exit 0
fi
