#!/usr/bin/python
#
# python implementation of mpgedit scramble.pl test program.
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

mpgedit = os.path.join(os.path.dirname(sys.argv[0]), 'mpgedit')

def _load_times(buffer):
    times = []
    prev = '0.000'
    i = 1
    for line in buffer.readlines():
        if line.startswith('t='):
            time = line.split()[0].split('=')[1][:-1]            
            times.append((i, prev, time))
            prev = time
            i = i + 1
    return times

def _randomize(times):    
    save = times.pop() # don't randomize the last entry
    length = len(times) - 1
    for swap in range(length * 5):
        rval1 = random.randint(0, length)
        rval2 = random.randint(0, length)
        times[rval1], times[rval2] = times[rval2], times[rval1]
    times.append(save)
    return times

def scramble_test(mp3file, output=None):    
    times = _randomize(_load_times(os.popen('"%s" -vv %s' % (mpgedit, mp3file)))) 
    f = output and file(output, 'w') or sys.stdout
    save = times.pop() # don't print time for last frame 
    for time in times:
        print >> f, '%d: %s-%s' % time
    times.append(save)
    return times
    
def scramble(mp3file, verbose):
    if os.path.exists('scramble.out'): os.remove('scramble.out')
    if os.path.exists('scramble.mp3'): os.remove('scramble.mp3')
    times = scramble_test(mp3file, 'scramble.out')
    save = times.pop()
    print 'Scrambling frame order in file %s to scramble.mp3' % mp3file
    i = 0
    group = times[i:i+40]
    while group:
        edits = ['-e%s-%s' % (time[1], time[2]) for time in group]        
        cmd = '"%s" -o %sscramble.mp3 %s %s' % (mpgedit, i and '+' or '', ' '.join(edits), mp3file)
        os.popen(cmd)
        if verbose: print cmd, '\n'
        else: sys.stdout.write('.')
        i += 40
        group = times[i:i+40]    

    # special treatment of last frame
    os.popen('mpgedit -o +scramble.mp3 -e%s- %s' % (save[1], mp3file))
    if verbose: print cmd, '\n'
    else: sys.stdout.write('.')
    print

       
def unscramble(verbose):
    mp3file = 'descramble.mp3'
    if os.path.exists(mp3file): os.remove(mp3file)
    if os.path.exists('scramble.idx'): os.remove('scramble.idx')
    i = 1
    times = {}
    for line in file('scramble.out'):      
        pos = int(line.split(':')[0])
        t1, t2 = line.split(': ')[1].split('-')
        times[pos] = (i, t1, t2.rstrip())
        i += 1
        
    times_len = len(times) + 1
    start = 1    
    stop  = min(start + 40, times_len)
    print 'Unscrambling frame order in scramble.mp3 to descramble.mp3'
    while start < stop:
        edits = ['-e%s-%s' % times[times[i][0]][1:] for i in range(start, stop)]
        cmd = '"%s" -s -o %s%s %s scramble.mp3' % (mpgedit, start != 1 and '+' or '', mp3file, ' '.join(edits))
        os.popen(cmd)
        if verbose: print cmd, '\n'
        else: sys.stdout.write('+')
        start += 40
        stop = min(start + 40, times_len)

        
    # special treatment of last frame
    cmd = 'mpgedit -s -o +%s -e%s- scramble.mp3' % (mp3file, times[times_len-1][2])
    os.popen(cmd)
    if verbose: print cmd, '\n'
    else: sys.stdout.write('+')
    print
    
def usage():
    print '''usage: scramble [option] ... mp3_file
--help       | -h          : print this help message and exit    
--test       | -t          : test (output to stdout if output not specified)
--output     | -o filename : output file (only used with test)
--scramble   | -s          : scrambles mp3_file and outputs to scramble.mp3;
                             this is the default command
--unscramble | -u          : unscrambles scramble.mp3; outputs to descramble.mp3
--verbose    | -v          : turns on verbose output'''


if __name__ == '__main__':
    import getopt

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hstuvo:",
                                   ['help', 'scramble', 'test', 'output=', 'unscramble', 'verbose'])
        arg = None
        if len(args): arg = args[0]
        output = None
        test = False            
        scram = True
        verbose = False
    except (getopt.GetoptError, IndexError):
        usage()
        sys.exit()
    
    for o, a in opts:
        if o in ['-h', '--help']:
            usage()
            sys.exit()
        if o in ['-u', '--unscramble']:
            scram = False
        elif o in ['-t', '--test']:
            test = True
        elif o in ['-o', '--output']:
            output = a
        elif o in ['-v', '--verbose']:
            verbose = True

    if test:
        scramble_test(arg, output)
    elif scram and arg:
        scramble(arg, verbose)
    elif not scram:
        unscramble(verbose)
    else:
        usage()
        
 
