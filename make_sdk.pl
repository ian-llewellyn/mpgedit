#!/usr/bin/perl
@COMMON=("../..|decoder.h", "../..|editif.h", "../..|header.h",
         "../..|mp3time.h", "../..|mpegindx.h", "../..|mpegstat.h",
         "../..|playif.h", "../..|portability.h", "../..|volumeif.h",
         "../..|xing_header.h", 
         "../../contrib/python/py_mpgedit|pympgedit.py",
         "../../contrib/python/py_mpgedit|test_pympgedit.py",
         "../../contrib/python/py_mpgedit|simple_play.py",
         "../../contrib/python/py_mpgedit|simple_edit.py", "../..|test1.mp3",
         "../../contrib/python/py_mpgedit|LICENSE.txt");

@WINDOWS=("../..|libmpgedit_decoder.dll", 
          "../..|mpgedit.dll",
          "../../mad/win32|libmpgedit_decoder.dll|libdecoder_mad.dll",

          "../..|mpgedit.lib", "../../../win32|win32/getopt.h",
          "../..|README_SDK_win32.txt|README_SDK.txt",
          "../..|README_win32_mpgedit.txt|README.txt",

          "../..|mpgedit_sdk.iss",
          "../..|ModifyPath.iss",
          "../../contrib/python/py_mpgedit/dist|py_mpgedit-0.3beta.win32.exe");

@WINDOWS_DIRS=("mpgedit_sdk", "mpgedit_sdk/win32", "mpgedit_sdk/win32/win32");

@UNIX=("../..|libmpgedit.so", 
       "../..|libdecoder_mpg123.so",
       "../..|libdecoder_mad.so",
       "../..|libdecoder_popen.so",
       "../../mpglib|libmpglib_mpgedit.so",
       "../..|install_sdk.sh|install.sh",
       "../..|README_SDK.linux|README");
@UNIX_DIRS=("mpgedit_sdk", "mpgedit_sdk/linux");


sub mksubdirs(@)
{
  my (@subdirs) = @_;
  my $i;

  for $i (@subdirs) {
    if (! -d $i) {
      mkdir("$i", 0755);
    }
  }
}

sub lnfiles(@)
{
  my (@files) = @_;
  my $i, $src, $dest, $file;

  for $i (@files) {
    $rename="";
#print("lnfiles: i='$i'\n");
    ($src, $dest, $rename) = split(/\|/, $i);
    ($file = $dest) =~ s|.*/||;
    if (length($rename) > 0) {
        $dest= $rename;
    }
#print("lnfiles: src='$src' dest=$dest file=$file\n");

#print("symlink('$src/$file', '$dest')\n");
    symlink("$src/$file", "$dest");
  }
}

$platform = "all";
if (length($ARGV[0]) > 0) {
  $version = $ARGV[0];
}
else {
  $version = "0-7p1";
}

if (length($ARGV[1]) > 0) {
  $platform = $ARGV[1];
}


# Tidy up
`rm -rf mpgedit_sdk`;

# make Windows SDK
if ($platform =~ /win32$/ || $platform =~ /all$/) {
  mksubdirs(@WINDOWS_DIRS);
  chdir("mpgedit_sdk/win32");
  lnfiles(@COMMON, @WINDOWS);
  chdir("../..");
  unlink("mpgedit_sdk.zip");
  print("zip -r mpgedit_sdk_win32_${version}.zip mpgedit_sdk/win32\n");
  @output=`zip -r mpgedit_sdk_win32_${version}.zip mpgedit_sdk/win32`;
  print @output;
}


# make Linux SDK
if ($platform =~ /linux$/ || $platform =~ /all$/) {
  mksubdirs(@UNIX_DIRS);
  chdir("mpgedit_sdk/linux");
  lnfiles(@COMMON, @UNIX);
  chdir("../..");
  unlink("mpgedit_sdk_linux.tgz");
  unlink("mpgedit_sdk_linux_tgz.zip");
  print("tar zcvhf mpgedit_sdk_linux_${version}.tgz mpgedit_sdk\n");
  @output=`tar zcvhf mpgedit_sdk_linux_${version}.tgz mpgedit_sdk/linux`;
  print("zip   mpgedit_sdk_linux_${version}_tgz.zip mpgedit_sdk_linux_${version}.tgz\n");
  @output=`zip mpgedit_sdk_linux_${version}_tgz.zip mpgedit_sdk_linux_${version}.tgz`;
  print @output;
}
