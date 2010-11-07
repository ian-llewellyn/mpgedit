#!/usr/bin/python

import sys
import pympgedit as mpgedit

def index_callback(frame, sec, msec, offset):
    print 'frame=%4d  time=%2d:%02d  offset=%6d' % (frame, sec, msec, offset)
    return True
 
def edit_callback(frame, sec, offset):
    print 'frame=%4d  sec=%2d  offset=%6d' % (frame, sec, offset)
    return True

def editif_test(mp3file):
    # This test edits a file by scrambling it, then puts it back together
    
    # Create a spec that contains several time slices of the original file
    spec = mpgedit.EditSpec()
    spec.append(mp3file, '15.986-19.983')
    spec.append(mp3file, '11.989-15.986')
    spec.append(mp3file, '7.993-11.989')
    spec.append(mp3file, '3.996-7.993')
    spec.append(mp3file, '0.0-3.996')
    spec.append(mp3file, '27.976-31.999')
    spec.append(mp3file, '23.979-27.976')
    spec.append(mp3file, '19.983-23.979')
    spec.append(mp3file, '31.999-34.0')

    # Create an index for the original mp3 file which will contain frame and time data
    print '\nCreating index for %s...' % mp3file
    mpgedit.Index(mp3file).index(index_callback)

    
    print '\nSpec entries' 
    print 'length=%d' % spec.length()    
    for s in spec:
        print 'filename=%s, stime=%s, etime=%s' % s
        
    # Create a new mp3 file based on the spec above      
    edit = mpgedit.Edit(spec, 'editif_test.mp3')
    print '\nCreating editif_test.mp3...'
    edit.edit()
    
    print 'info: frames=%d, sec=%d, offset=%d' % (edit.frames(), edit.sec(), edit.offset())
    print 'xing header:', edit.xing_header()

    # Create a new spec that will unscramble the scrambled mp3 file
    spec = mpgedit.EditSpec()
    tmpfile = 'editif_test.mp3'
    spec.append(tmpfile, "15.986-19.983")
    spec.append(tmpfile, "11.989-15.986")
    spec.append(tmpfile, "7.993-11.989")
    spec.append(tmpfile, "3.996-7.993")
    spec.append(tmpfile, "0.0-3.996")
    spec.append(tmpfile, "28.002-31.999")
    spec.append(tmpfile, "24.006-28.002")
    spec.append(tmpfile, "19.983-24.006")
    spec.append(tmpfile, "31.999-")
    
    # Create an index for scrambled mp3 file
    mpgedit.Index(tmpfile).index()    
    print '\nCreating editif_test2.mp3...'
    
    # Create a new mp3 file which will be exactly the same as the original.
    mpgedit.Edit(spec, 'editif_test2.mp3').edit(edit_callback)
    
                                                                          
if __name__ == '__main__':
    editif_test(sys.argv[1])       
    
    
