Summary: An MPEG 1 layer 1/2/3 (mp3), MPEG 2, and MPEG 2.5 audio file editor.
Name: mpgedit
Version: __MAJOR__.__MINOR__
Release: __CKPOINT__
License: GPL
Group: Applications/Multimedia
Source: http://mpgedit.org/mpgedit/download_dev/mirror1/mpgedit___RELEASE___src.tgz
BuildRoot: /var/tmp/%{name}-buildroot
Vendor: mpgedit.org

%description
mpgedit is an MPEG 1 layer 1/2/3 (mp3), MPEG 2, and MPEG 2.5 audio file
editor that is capable of processing both Constant Bit Rate (CBR) and
Variable Bit Rate (VBR) encoded files. mpgedit can cut an input MPEG file
into one or more output files, as well as join one or more input MPEG files
into a single output file. Since no file decoding / encoding occurs during
editing, there is no audio quality loss when editing with mpgedit. When
editing VBR files that have a XING header, mpgedit updates the output file's
XING header information to reflect the new file size and average bit rate.

%prep
rm -rf __RELEASE__src
tar zxvf %_sourcedir/mpgedit___RELEASE___src.tgz
cd mpgedit___RELEASE___src/mpgedit
cp makefile.linux makefile
./make_alsa.sh

%build 
cd mpgedit___RELEASE___src/mpgedit
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
cd mpgedit___RELEASE___src/mpgedit
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/lib
mkdir -p $RPM_BUILD_ROOT/usr/share/doc
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/mpgedit

install -s -m 755 mpgedit $RPM_BUILD_ROOT/usr/bin/mpgedit
install -s -m 755 mpgedit_nocurses $RPM_BUILD_ROOT/usr/bin/mpgedit_nocurses
install -m 755 mp3decoder.sh $RPM_BUILD_ROOT/usr/bin/mp3decoder.sh

install -m 755 libdecoder_mpg123.so $RPM_BUILD_ROOT/usr/lib/libdecoder_mpg123.so
install -m 755 libdecoder_popen.so $RPM_BUILD_ROOT/usr/lib/libdecoder_popen.so
install -m 755 libdecoder_mad.so $RPM_BUILD_ROOT/usr/lib/libdecoder_mad.so
install -m 755 libmpglib_mpgedit.so $RPM_BUILD_ROOT/usr/lib/libmpglib_mpgedit.so
# ln -s $RPM_BUILD_ROOT/usr/lib/libdecoder_mpg123.so $RPM_BUILD_ROOT/usr/lib/libmpgedit_decoder.so



install -m 644 README $RPM_BUILD_ROOT/usr/share/doc/mpgedit/README
install -m 644 TODO $RPM_BUILD_ROOT/usr/share/doc/mpgedit/TODO
install -m 644 COPYING $RPM_BUILD_ROOT/usr/share/doc/mpgedit/COPYING
install -m 644 THANKS $RPM_BUILD_ROOT/usr/share/doc/mpgedit/THANKS
install -m 644 ChangeLog $RPM_BUILD_ROOT/usr/share/doc/mpgedit/ChangeLog

%clean
cd mpgedit___RELEASE___src/mpgedit
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/mpgedit
/usr/bin/mpgedit_nocurses
/usr/bin/mp3decoder.sh
/usr/lib/libdecoder_mpg123.so
/usr/lib/libdecoder_popen.so
/usr/lib/libdecoder_mad.so
/usr/lib/libmpglib_mpgedit.so

%doc 
%defattr(-,root,root)
/usr/share/doc/mpgedit/README
/usr/share/doc/mpgedit/TODO
/usr/share/doc/mpgedit/COPYING
/usr/share/doc/mpgedit/THANKS
/usr/share/doc/mpgedit/ChangeLog

%post
ln -f -s /usr/lib/libdecoder_mpg123.so /usr/lib/libmpgedit_decoder.so

%postun
[ -f /usr/lib/libdecoder_mpg123.so ] || rm -f /usr/lib/libmpgedit_decoder.so
sh -c 'rm -f /usr/share/doc/xmpgedit     > /dev/null 2>&1; exit 0'
sh -c 'rmdir /usr/share/doc/xmpgedit     > /dev/null 2>&1; exit 0'

%changelog
