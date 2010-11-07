Summary: An MPEG 1 layer 1/2/3 (mp3), MPEG 2, and MPEG 2.5 audio file editor.
Name: mpgedit-man
Version: __MAJOR__.__MINOR__
Release: __CKPOINT__
Copyright: GPL
Group: Applications/Multimedia
Source: http://mpgedit.org/mpgedit/download_dev/mirror1/mpgedit___RELEASE___src.exe
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
/usr/src/redhat/SOURCES/mpgedit___RELEASE___src.exe -x
cd mpgedit___RELEASE___src/mpgedit
cp makefile.linux makefile

%build 
cd mpgedit___RELEASE___src/mpgedit
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
cd mpgedit___RELEASE___src/mpgedit
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
mkdir -p $RPM_BUILD_ROOT/usr/share/doc
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/mpgedit

install -m 644 mpgedit.1 $RPM_BUILD_ROOT/usr/share/man/man1/mpgedit.1
install -m 644 xmpgedit.1 $RPM_BUILD_ROOT/usr/share/man/man1/xmpgedit.1
install -m 644 decoder.so.1 $RPM_BUILD_ROOT/usr/share/man/man1/decoder.so.1
install -m 644 mp3decoder.sh.1 $RPM_BUILD_ROOT/usr/share/man/man1/mp3decoder.sh.1
install -m 644 scramble.pl.1 $RPM_BUILD_ROOT/usr/share/man/man1/scramble.pl.1
install -m 644 unscramble.pl.1 $RPM_BUILD_ROOT/usr/share/man/man1/unscramble.pl.1
install -m 644 scramble_times.pl.1 $RPM_BUILD_ROOT/usr/share/man/man1/scramble_times.pl.1

gzip $RPM_BUILD_ROOT/usr/share/man/man1/mpgedit.1
gzip $RPM_BUILD_ROOT/usr/share/man/man1/xmpgedit.1
gzip $RPM_BUILD_ROOT/usr/share/man/man1/decoder.so.1
gzip $RPM_BUILD_ROOT/usr/share/man/man1/mp3decoder.sh.1
gzip $RPM_BUILD_ROOT/usr/share/man/man1/scramble.pl.1
gzip $RPM_BUILD_ROOT/usr/share/man/man1/unscramble.pl.1
gzip $RPM_BUILD_ROOT/usr/share/man/man1/scramble_times.pl.1

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
%doc 
/usr/share/doc/mpgedit/README
/usr/share/doc/mpgedit/TODO
/usr/share/doc/mpgedit/COPYING
/usr/share/doc/mpgedit/THANKS
/usr/share/doc/mpgedit/ChangeLog

/usr/share/man/man1/mpgedit.1.gz
/usr/share/man/man1/xmpgedit.1.gz
/usr/share/man/man1/decoder.so.1.gz
/usr/share/man/man1/mp3decoder.sh.1.gz
/usr/share/man/man1/scramble.pl.1.gz
/usr/share/man/man1/unscramble.pl.1.gz
/usr/share/man/man1/scramble_times.pl.1.gz

%post
count=`grep "^MANPATH.*/usr/share/man" "/etc/man.config" | wc -l`
[ $count -eq 0 ] && echo "MANPATH       /usr/share/man" >> /etc/man.config
env PATH=$PATH:/usr/sbin:/usr/bin makewhatis

%postun

%changelog
