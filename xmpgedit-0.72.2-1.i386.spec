Summary: An MPEG 1 layer 1/2/3 (mp3), MPEG 2, and MPEG 2.5 audio file editor.
Name: xmpgedit
Version: __MAJOR__.__MINOR__
Release: __CKPOINT__
Copyright: GPL
Group: Applications/Multimedia
Source: http://mpgedit.org/mpgedit/download_dev/mirror1/mpgedit___RELEASE___src.exe
BuildRoot: /var/tmp/%{name}-buildroot
Vendor: mpgedit.org

%description
xmpgedit is the graphical user interface (GUI) implementation of mpgedit.
xmpgedit supports most of the same editing features of mpgedit.  xmpgedit
uses the GTK+ 2.0 toolkit, on all platforms.

mpgedit is an MPEG 1 layer 1/2/3 (mp3), MPEG 2, and MPEG 2.5 audio file
editor that is capable of processing both Constant Bit Rate (CBR) and
Variable Bit Rate (VBR) encoded files. mpgedit can cut an input MPEG file
into one or more output files, as well as join one or more input MPEG files
into a single output file. Since no file decoding / encoding occurs during
editing, there is no audio quality loss when editing with mpgedit. When
editing VBR files that have a XING header, mpgedit updates the output file's
XING header information to reflect the new file size and average bit rate.

%prep
/usr/src/redhat/SOURCES/mpgedit___RELEASE___src.exe -x
cd mpgedit___RELEASE___src/mpgedit
cp makefile.linux makefile

%build 
cd mpgedit___RELEASE___src/mpgedit
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
cd mpgedit___RELEASE___src/mpgedit
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/lib
mkdir -p $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit
mkdir -p $RPM_BUILD_ROOT/usr/share/doc
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/xmpgedit

install -s -m 755 gui/xmpgedit $RPM_BUILD_ROOT/usr/bin/xmpgedit
install -m 755 mp3decoder.sh $RPM_BUILD_ROOT/usr/bin/mp3decoder.sh

install -m 755 libdecoder_mpg123.so $RPM_BUILD_ROOT/usr/lib/libdecoder_mpg123.so
install -m 755 libdecoder_popen.so $RPM_BUILD_ROOT/usr/lib/libdecoder_popen.so
install -m 755 libdecoder_mad.so $RPM_BUILD_ROOT/usr/lib/libdecoder_mad.so
install -m 755 libmpglib_mpgedit.so $RPM_BUILD_ROOT/usr/lib/libmpglib_mpgedit.so
ln -s $RPM_BUILD_ROOT/usr/lib/libdecoder_mpg123.so $RPM_BUILD_ROOT/usr/lib/libmpgedit_decoder.so

install -m 644 gui/blankdigit_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/blankdigit_led.xpm
install -m 644 gui/blankpunct_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/blankpunct_led.xpm
install -m 644 gui/close.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/close.xpm
install -m 644 gui/colon_hour_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/colon_hour_led.xpm
install -m 644 gui/colon_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/colon_led.xpm
install -m 644 gui/colon_minute_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/colon_minute_led.xpm
install -m 644 gui/eight_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/eight_led.xpm
install -m 644 gui/eject.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/eject.xpm
install -m 644 gui/five_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/five_led.xpm
install -m 644 gui/four_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/four_led.xpm
install -m 644 gui/next_t.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/next_t.xpm
install -m 644 gui/nine_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/nine_led.xpm
install -m 644 gui/one_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/one_led.xpm
install -m 644 gui/pause.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/pause.xpm
install -m 644 gui/period_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/period_led.xpm
install -m 644 gui/period_second_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/period_second_led.xpm
install -m 644 gui/play.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/play.xpm
install -m 644 gui/record_green.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/record_green.xpm
install -m 644 gui/record_red.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/record_red.xpm
install -m 644 gui/record.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/record.xpm
install -m 644 gui/seven_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/seven_led.xpm
install -m 644 gui/six_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/six_led.xpm
install -m 644 gui/stop.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/stop.xpm
install -m 644 gui/three_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/three_led.xpm
install -m 644 gui/two_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/two_led.xpm
install -m 644 gui/volume1.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/volume1.xpm
install -m 644 gui/zero_led.xpm $RPM_BUILD_ROOT/usr/share/pixmaps/xmpgedit/zero_led.xpm

install -m 644 README $RPM_BUILD_ROOT/usr/share/doc/xmpgedit/README
install -m 644 TODO $RPM_BUILD_ROOT/usr/share/doc/xmpgedit/TODO
install -m 644 COPYING $RPM_BUILD_ROOT/usr/share/doc/xmpgedit/COPYING
install -m 644 THANKS $RPM_BUILD_ROOT/usr/share/doc/xmpgedit/THANKS
install -m 644 ChangeLog $RPM_BUILD_ROOT/usr/share/doc/xmpgedit/ChangeLog

%clean
cd mpgedit___RELEASE___src/mpgedit
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc 
/usr/share/doc/xmpgedit/README
/usr/share/doc/xmpgedit/TODO
/usr/share/doc/xmpgedit/COPYING
/usr/share/doc/xmpgedit/THANKS
/usr/share/doc/xmpgedit/ChangeLog

/usr/bin/xmpgedit
/usr/bin/mp3decoder.sh

/usr/lib/libdecoder_mpg123.so
/usr/lib/libdecoder_mad.so
/usr/lib/libmpglib_mpgedit.so

/usr/share/pixmaps/xmpgedit/blankdigit_led.xpm
/usr/share/pixmaps/xmpgedit/blankpunct_led.xpm
/usr/share/pixmaps/xmpgedit/close.xpm
/usr/share/pixmaps/xmpgedit/colon_hour_led.xpm
/usr/share/pixmaps/xmpgedit/colon_led.xpm
/usr/share/pixmaps/xmpgedit/colon_minute_led.xpm
/usr/share/pixmaps/xmpgedit/eight_led.xpm
/usr/share/pixmaps/xmpgedit/eject.xpm
/usr/share/pixmaps/xmpgedit/five_led.xpm
/usr/share/pixmaps/xmpgedit/four_led.xpm
/usr/share/pixmaps/xmpgedit/next_t.xpm
/usr/share/pixmaps/xmpgedit/nine_led.xpm
/usr/share/pixmaps/xmpgedit/one_led.xpm
/usr/share/pixmaps/xmpgedit/pause.xpm
/usr/share/pixmaps/xmpgedit/period_led.xpm
/usr/share/pixmaps/xmpgedit/period_second_led.xpm
/usr/share/pixmaps/xmpgedit/play.xpm
/usr/share/pixmaps/xmpgedit/record_green.xpm
/usr/share/pixmaps/xmpgedit/record_red.xpm
/usr/share/pixmaps/xmpgedit/record.xpm
/usr/share/pixmaps/xmpgedit/seven_led.xpm
/usr/share/pixmaps/xmpgedit/six_led.xpm
/usr/share/pixmaps/xmpgedit/stop.xpm
/usr/share/pixmaps/xmpgedit/three_led.xpm
/usr/share/pixmaps/xmpgedit/two_led.xpm
/usr/share/pixmaps/xmpgedit/volume1.xpm
/usr/share/pixmaps/xmpgedit/zero_led.xpm

%post
ln -f -s /usr/lib/libdecoder_mpg123.so /usr/lib/libmpgedit_decoder.so

%postun
[ -f /usr/lib/libdecoder_mpg123.so ] || rm -f /usr/lib/libmpgedit_decoder.so

%changelog
