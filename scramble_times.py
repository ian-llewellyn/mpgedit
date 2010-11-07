#!/usr/bin/python
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

