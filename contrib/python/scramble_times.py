#!/usr/bin/python
#
# python implementation of mpgedit scramble_times.pl test program.
#
# Copyright (C) 2003 Bryan Weingarten
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA  02111-1307  USA.
#
import sys
import os
import random

def load_times(buffer):
    times = []
    prev = '0.000'
    i = 1
    for line in buffer.readlines():
        if line.startswith('t='):
            time = line.split()[0].split('=')[1][:-1]
            times.append('%d: %s-%s\n' % (i, prev, time))
            prev = time
            i = i + 1
    return times


def randomize(times):
    length = len(times) - 1
    for swap in range(length * 5):
        rval1 = random.randint(0, length)
        rval2 = random.randint(0, length)
        times[rval1], times[rval2] = times[rval2], times[rval1]


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print 'usage: scramble_times mp3_file [outfile]'
        sys.exit()

    child_out = os.popen('mpgedit -vv ' + sys.argv[1])
    times = load_times(child_out)
    randomize(times)
    if len(sys.argv) > 2:
        file(sys.argv[2], 'w').writelines(times)
    for time in times:
        print time,

