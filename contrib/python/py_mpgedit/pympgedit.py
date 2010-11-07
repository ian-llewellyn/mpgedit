from ctypes import *
import threading
import sys
import traceback

if sys.platform == 'win32':
    _m = cdll.mpgedit
else:
    _m = cdll.LoadLibrary("libmpgedit.so")

_CALLBACK = CFUNCTYPE(c_int, c_void_p, c_long, c_long)

class _mpeg_time(Structure):
    _fields_ = [("units", c_int),
                ("usec", c_int)]

_m.mpgedit_editspec_get_file.restype = c_char_p
_m.mpgedit_editspec_get_stime.restype = POINTER(_mpeg_time)
_m.mpgedit_editspec_get_etime.restype = POINTER(_mpeg_time)

class MpgeditError(Exception): pass

class Play(object):
    def __init__(self, mp3file):
        self._ctx = _m.mpgedit_play_init(mp3file, 0)
        _m.mpgedit_play_volume_init(self._ctx, -1, -1)
        self.error = None
        self.playback = False

    def __del__(self):
        self.close()

    def close(self):
        if self._ctx:
            _m.mpgedit_play_close(self._ctx)
            self._ctx = None
            self.playback = False

    def stop(self):
        self.playback = False

    def play(self, callback=None):
        if self.playback:
            return None

        if not self._ctx:
            return None

        def _play(callback=callback):
            self.error = None

            def _callback_wrapper(data, sec, msec):
                try:
                    if not self.playback:
                        return 0
                    if callback:
                        return callback(sec, msec)
                    else:
                        return 1
                except (Exception, KeyboardInterrupt), e:
                    self.error = sys.exc_info()[0]('\n' + traceback.format_exc())

            _m.mpgedit_play_set_status_callback(self._ctx, _CALLBACK(_callback_wrapper), 0)
            while(_m.mpgedit_play_frame(self._ctx)): pass
            if self.error: raise self.error

        t = threading.Thread(target=_play)
        t.start()
        self.playback = True
        return t

        
    def seek_time(self, sec, msec):
        return _m.mpgedit_play_seek_time(self._ctx, sec, msec)

    def total_size(self):
        return _m.mpgedit_play_total_size(self._ctx)

    def total_time(self):
        return _m.mpgedit_play_total_sec(self._ctx), _m.mpgedit_play_total_msec(self._ctx)
        
    def current_time(self):
        return _m.mpgedit_play_current_sec(self._ctx), _m.mpgedit_play_current_msec(self._ctx)
        
    def get_volume(self):
        left, right = c_int(), c_int()
        if _m.mpgedit_play_volume_get(self._ctx, byref(left), byref(right)) == -1:
            raise ValueError('Unable to retrieve volume information')
        return left.value, right.value

    def set_volume(self, left, right=-1):
        if right == -1: right = left
        _m.mpgedit_play_volume_set(self._ctx, left, right)


class Index(object):
    def __init__(self, mp3file):
        self._ctx = _m.mpgedit_edit_index_init(mp3file)

    def __del__(self):
        self.close()

    def close(self):
        if self._ctx:
            _m.mpgedit_edit_index_free(self._ctx);
            self._ctx = None
            
    def index(self, callback=None):
        if not callback: callback = lambda *args: 1

        while(_m.mpgedit_edit_index(self._ctx) == 0):
            if not callback(_m.mpgedit_edit_index_frames(self._ctx),
                            _m.mpgedit_edit_index_sec(self._ctx),
                            _m.mpgedit_edit_index_msec(self._ctx),
                            _m.mpgedit_edit_index_offset(self._ctx)): break
            
    def frames(self):
        return _m.mpgedit_edit_index_frames(self._ctx)

    def time(self):
        return _m.mpgedit_edit_index_sec(self._ctx), _m.mpgedit_edit_index_msec(self._ctx)

    def offset(self):
        return _m.mpgedit_edit_index_offset(self._ctx)


class EditSpec(object):
    def __init__(self):
        self._ctx = _m.mpgedit_editspec_init()

    def __del__(self):
        self.close()

    def close(self):
        if self._ctx:
            _m.mpgedit_editspec_free(self._ctx)
            self._ctx = None

    def append(self, mp3file, start_time=None, end_time=None):
       _m.mpgedit_editspec_append(self._ctx, mp3file,
           ('%s-%s' % (start_time or '', end_time or '')).strip('-'))

    def __iadd__(self, spec):
        '''append a time a time spec value

        value is a 2-tuple (mp3file, start_time)
        or a 3-tuple (mp3file, start_time, end_time)'''
        self.append(*spec)
        return self

    def length(self):
        return _m.mpgedit_editspec_get_length(self._ctx)

    def filename(self, offset):
        file = _m.mpgedit_editspec_get_file(self._ctx, offset)
        if not file: raise IndexError('offset is out of range')
        return file

    def start_time(self, offset):
        t = _m.mpgedit_editspec_get_stime(self._ctx, offset)
        if not t: raise IndexError('offset is out of range')
        return t[0].units, t[0].usec

    def end_time(self, offset):
        t = _m.mpgedit_editspec_get_etime(self._ctx, offset)
        if not t: raise IndexError('offset is out of range')
        return t[0].units, t[0].usec

    def __getitem__(self, offset):
        return self.filename(offset), self.start_time(offset), self.end_time(offset)

    def __iter__(self):
        for offset in xrange(self.length()):
            yield self.filename(offset), self.start_time(offset), self.end_time(offset)
        raise StopIteration()


edit_errors = {1: 'Unable to append to output file',
               2: 'Output file exists',
               3: 'Unable to read input file',
               4: 'Unable to read index file',
               5: 'Unable to open output file',
               6: 'Invalid Edit object',
               7: 'Bad context'}


class Edit(object):
    def __init__(self, editspec, outfile, flags=0):
        rstatus = c_int()
        self._ctx = _m.mpgedit_edit_files_init(editspec._ctx, outfile, flags, byref(rstatus))
        if rstatus.value != 0:
            raise MpgeditError(rstatus.value, edit_errors.get(rstatus.value, 'Unknown error'))

    def __del__(self):
        self.close()

    def close(self):
        if self._ctx:
            _m.mpgedit_edit_files_free(self._ctx)
            self._ctx = None


    def edit(self, callback=None):
        if not callback:
           callback = lambda *args: 1

        rstatus = c_int()

        while _m.mpgedit_edit_files(self._ctx, byref(rstatus)) != 0:
            if not callback(_m.mpgedit_edit_frames(self._ctx),
                            _m.mpgedit_edit_sec(self._ctx),
                            _m.mpgedit_edit_offset(self._ctx)):
                break

        if rstatus.value != 0:
            raise MpgeditError(rstatus.value, edit_errors.get(rstatus.value, 'Unknown error'))



    def frames(self):
        return _m.mpgedit_edit_frames(self._ctx)

    def sec(self):
        return _m.mpgedit_edit_sec(self._ctx)

    def offset(self):
        return _m.mpgedit_edit_offset(self._ctx)

    def xing_header(self):
        header_dict = {}
        if _m.mpgedit_edit_has_xing(self._ctx):
            header = create_string_buffer(1024)
            _m.xingheader2str(_m.mpgedit_edit_xing_header(self._ctx), header)
            for line in header.value.splitlines():
                n, v = line.split('=')
                header_dict[n.strip()] = int(v.strip())
        return header_dict
