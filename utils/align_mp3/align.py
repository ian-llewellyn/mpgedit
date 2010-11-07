#!/usr/bin/python
#
# Script to join a set of overlapping mp3 files.  The expected structure
# of these files is each following file starts a few seconds before
# the previous file ends.  The purpose of this script is to find the
# frame where this intersection occurs.  After this analysis is
# complete, the set of files are joined together, saving the edits
# into 'join.mp3'
#
import os
import sys
import getopt

# sample data line
#
# MPEG header t=6.661s  br=112  sz=365  fr=255    pos=92892      \
#  pos=0x16adc   md5sum=0xc596f7e090f4944aa17d8b3c0fe8d299

def align_mp3(file1, file2, offset):
  frame_haystack = {}
  sum_haystack   = {}

  # The first frame of file1 is the needle to look for

  command    = 'mpgedit -vvvv -E -e%s-%s.026 %s' % (offset, offset, file1)

  needle_list = [line for line in os.popen(command)
                if line.startswith('MPEG header t=')]

  command    = 'mpgedit ' + file2
  list       = [line for line in os.popen(command)
               if line.startswith('Track length: ')]
  track_len  = list[0].split()[3]
  track_len  = track_len.lstrip('(')
  track_len  = track_len.rstrip('s)')

#  # search 10 minutes of the file...
#  start_time = int(track_len) - 600
#  if start_time < 0: start_time = 0
  start_time = int(track_len) / 2
  start_time = 0
  print "start_time is %d" % start_time

  # Generate the dictionary "haystacks" to search for needle in

  command = 'mpgedit -vvvv -E -e%d- %s' %(start_time, file2)
  for line in os.popen(command):
    if line.startswith('MPEG header t='):
      fields                    = line.split()
      sum_haystack[fields[8]]   = fields[5]
      frame_haystack[fields[5]] = fields[2]

  for i in range(0, len(needle_list)):
    needle_sum = needle_list[i].split()[8]
    try:
      needle = sum_haystack[needle_sum]
      print "found needle='%s'" %needle
    except KeyError:
      print "didnt find needle at index %d"  % i

  previous_frame = int(needle.lstrip('fr=')) - 1
  if previous_frame < 0: previous_frame = 0
  previous_frame = 'fr=' + str(previous_frame)

  edit_time = frame_haystack[previous_frame]
  end_time  = edit_time.lstrip('t=')
  return end_time.rstrip('s')


def usage(errstr):
  print errstr
  print "usage: %s [-o offset] f1.mp3 f2.mp3 ..." % (os.path.split(sys.argv[0])[1])
  os._exit(1)


if __name__ == "__main__":
  edit_times = []
  offset = "0"

  if (len(sys.argv) < 3):
    usage('at least two arguments required')
    os._exit(1)

  if (os.access("join.mp3", os.F_OK)):
    print "ERROR: Join file 'join.mp3' already exists"
    os._exit(1)

  try:
    opt, argv = getopt.getopt(sys.argv[1:], "o:")
  except getopt.error, v:
    print v
    sys.exit(1)


  for option, argument in opt:
    if option == "-o":
      offset = argument

  first_name = argv[0]
  for indx in range(1, len(argv)):
    second_name = argv[indx]
    try: 
      print 'Aligning ' + second_name + '...'
      end_time = align_mp3(second_name, first_name, offset)
    except KeyError:
      print 'Error: alignment of "%s" and "%s" failed' % \
            (first_name, second_name)
      os._exit(1)
    print "Alignment time: %s\n" % end_time

    edit_times.append((first_name, end_time))
    first_name = second_name

  edit_cmd = "mpgedit -s -o join.mp3 \\\n"
  for file, end_time in edit_times:
    edit_cmd += " -e%s-%s -f %s \\\n" %(offset, end_time, file)
  edit_cmd += " -e%s- -f %s" % (offset, argv[-1])

  print "Performing edit...\n%s\n" % edit_cmd
  os.system(edit_cmd)
