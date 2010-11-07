#!/usr/bin/python
import sys
import pympgedit as m


def play_callback_return_0(sec, msec):
    print sec, msec
    if sec == 6: return 0
    return 1


def index_callback(frame, sec, msec, offset):
    print frame, sec, msec, offset
    return 1


def edit_callback(frame, sec, offset):
    print frame, sec, offset
    return 1


def test_index():
    print
    print '#' * 50
    print '### test index'
    print

    index = m.Index('test1.mp3')
    index.index(index_callback)
    print 'frames', index.frames()
    print 'offset', index.offset()
    print 'time', index.time()


def test_editspec(mp3file):
    print
    print '#' * 50
    print '### test edit spec'
    print

    spec = m.EditSpec()
    spec += (mp3file, '15.986', '19.983')
    spec += (mp3file, 11.989, 15.986)
    spec += (mp3file, '7.993', '11.989')
    spec += (mp3file, '3.996', '7.993')
    spec += (mp3file, '0.0', '3.996')
    spec += (mp3file, 27.976, 31.999)
    spec.append(mp3file, '23.979', '27.976')
    spec.append(mp3file, 19.983, 23.979)
    spec.append(mp3file, '31.999', '34.0')
    
    print 'Spec entries'
    print 'length =', spec.length()
    print 'filename =', spec.filename(0)
    print 'start time =', spec.start_time(0)
    print 'end time =', spec.end_time(0)
    print 'spec[0]', spec[0]

    for i in range(5):
        print 'spec[%d]' % i, spec[i]

    for s in spec:
        print  s

    return spec
    
    
def test_edit(editspec):
    print
    print '#' * 50
    print '### test edit'
    print
    edit = m.Edit(editspec, 'test_edit.mp3')
    edit.edit(edit_callback)
    print 'frames', edit.frames()
    print 'sec', edit.sec()
    print 'offset', edit.offset()
    print 'xing header', edit.xing_header()

def test_play(mp3_file):
    print
    print '#' * 50
    print '### test play'
    print
    play = m.Play(mp3_file)
    play.play(play_callback_return_0).join()
    play.close()


if __name__ == '__main__':
    test_index()
    editspec = test_editspec('test1.mp3')
    test_edit(editspec)
    test_play('test1.mp3')
