#!/usr/bin/python
import sys
import time
import pympgedit as mpgedit
import os

class DemoPlay(mpgedit.Play):
    def __init__(self, mp3file):
        mpgedit.Index(mp3file).index()
        mpgedit.Play.__init__(self, mp3file)
        self.count = 0
        ttime = self.total_time()
        print 'Total: size=%d time=%d.%03d' % (self.total_size(), ttime[0], ttime[1])
        print 'Previous volume: left=%d right=%d' % self.get_volume()
        self.set_volume(25, 25)
        print 'New volume: left=%d right=%d' % self.get_volume()

    def status(self, sec, msec):
        self.count += 1
        sys.stdout.write('Frame %5d: %02d.%03d sec\r' % (self.count, sec, msec))
        sys.stdout.flush()
        return 1

    def play(self):
        mpgedit.Play.play(self, self.status)
        #
        # playback happens in a new thread; sleep for playback to finish...
        #
        time.sleep(self.total_time()[0] + 1)

    def stop(self):
        print 'stopping playback...'
        mpgedit.Play.stop(self)


try:
    infile = sys.argv[1]
    if not os.path.exists(infile):
        print "Error: file '%s' not found" %infile
        sys.exit('usage: %s mp3_file' %infile)
    p = DemoPlay(infile)
except IndexError:
        sys.exit('usage: %s mp3_file' %sys.argv[0])

if not os.path.exists(sys.argv[1]):
    print "Error: file '%s' not found" %sys.argv[1]
    sys.exit('usage: %s mp3_file' %sys.argv[0])

try:
    p.play()
except KeyboardInterrupt:
    print 'KeyboardInterrupt received, calling p.stop...'
    p.stop()
