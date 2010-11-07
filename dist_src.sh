#!/bin/sh

if [ -n "$1" ]; then
  RELEASE="$1"
fi
if [ -n "$2" ]; then
  TAG="$2"
fi

if [ -z "$RELEASE" -o -z "$TAG" ]; then
  # dist_parse_version.sh sets "RELEASE" from version.h
  . `dirname $0`/dist_parse_version.sh $0
fi

if [ -L mpgedit_${RELEASE}_src.tgz -a -L mpgedit_${RELEASE}_src.exe ]; then
  echo "$0 Source packages already exist"
  exit 0
fi

project_dir="mpgedit_${RELEASE}_src"
dir=../..
if [ -d $dir/$project_dir ]; then
  echo "Distribution directory '$dir/$project_dir' already exists"
  echo "    Remove and run this script again to regenerate"
  if [ ! -s mpgedit_${RELEASE}_src.exe ]; then
    ln -s $dir/mpgedit_${RELEASE}_src.exe .
  fi
  if [ ! -s mpgedit_${RELEASE}_src.tgz ]; then
    ln -s $dir/mpgedit_${RELEASE}_src.tgz .
  fi
else
  (cd $dir; mkdir $project_dir)
  if [ -n "$TAG" ]; then
    (cd $dir/$project_dir; cvs export -r $TAG mpgedit
     cd mpgedit/mad/include
     ln -s ../src/mad.h mad.h
    )
  else
    (cd $dir/$project_dir; cvs export mpgedit
     cd mpgedit/mad/include
     ln -s ../src/mad.h mad.h
    )
  fi

# Pre-process all template files. The README files are important
# for the source distribution here.
#
(cd $dir/mpgedit_${RELEASE}_src/mpgedit; ./mk_mpgedit_iss_readme.sh)

(
  cd $dir
  if [ ! -s mpgedit_${RELEASE}_src.tgz ]; then
    echo "Creating '$dir/mpgedit_${RELEASE}_src.tgz'..."
    tar zcf mpgedit_${RELEASE}_src.tgz mpgedit_${RELEASE}_src
  fi
  if [ ! -s mpgedit_${RELEASE}_src.exe ]; then
    echo "Creating '$dir/mpgedit_${RELEASE}_src.exe'..."
    src/mpgedit/make_shtgzexe.sh mpgedit_${RELEASE}_src.tgz
  fi
)

  ln -s $dir/mpgedit_${RELEASE}_src.exe .
  ln -s $dir/mpgedit_${RELEASE}_src.tgz .
fi
