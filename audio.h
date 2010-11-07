/* 
 * Audio 'LIB' defines
 */

#define AUDIO_OUT_HEADPHONES       0x01
#define AUDIO_OUT_INTERNAL_SPEAKER 0x02
#define AUDIO_OUT_LINE_OUT         0x04

enum { DECODE_TEST, DECODE_AUDIO, DECODE_FILE, DECODE_BUFFER, DECODE_WAV,
	DECODE_AU,DECODE_CDR,DECODE_AUDIOFILE };

#define AUDIO_FORMAT_MASK	  0x100
#define AUDIO_FORMAT_16		  0x100
#define AUDIO_FORMAT_8		  0x000

#define AUDIO_FORMAT_SIGNED_16    0x110
#define AUDIO_FORMAT_UNSIGNED_16  0x120
#define AUDIO_FORMAT_UNSIGNED_8   0x1
#define AUDIO_FORMAT_SIGNED_8     0x2
#define AUDIO_FORMAT_ULAW_8       0x4
#define AUDIO_FORMAT_ALAW_8       0x8

/* 3% rate tolerance */
#define AUDIO_RATE_TOLERANCE	  3

#if 0
#if defined(HPUX) || defined(SUNOS) || defined(SOLARIS) || defined(OSS) || defined(__NetBSD__) || defined(SPARCLINUX) || defined(__FreeBSD__)
#endif
#endif

#ifdef WIN32
typedef struct _volume_control_handle {
    int          fd;     /* UNIX fd/Win32 handle to mixer device */
    unsigned int id;     /* Win32 volume control ID              */
} volume_control_handle;
#endif /* WIN32 */

#ifdef __macosx
#ifndef MOSX_USES_AUDIOUNIT
#define MOSX_USES_AUDIOUNIT 1
#endif
#ifndef MOSX_AUDIOUNIT_V2
#define MOSX_AUDIOUNIT_V2 1
#endif
#if MOSX_USES_AUDIOUNIT
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/DefaultAudioOutput.h>
#else
#include <CoreAudio/AudioHardware.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>

struct aBuffer
{
        float * buffer;
        long size;

        float * ptr;    /* Where in the buffer are we? */
        long remaining;

        struct aBuffer * next;
};
typedef struct aBuffer aBuffer;


struct anEnv
{
        long size;
        short * debut;
        short * ptr;
        #if MOSX_USES_AUDIOUNIT
        AudioUnit output;
        #else
        AudioDeviceID output;
        #endif
        char play;

        /* Intermediate buffers */

        sem_t * semaphore; 
        aBuffer * from; /* Current buffers */
        aBuffer * to;
};
#endif  /* __macosx */

#define AUDIO_USES_FD

#ifdef SGI
/* #include <audio.h> */
#include <dmedia/audio.h>
#endif


#ifdef ALSA
#include <sys/asoundlib.h>
#endif

struct audio_info_struct
{
#ifdef AUDIO_USES_FD
  int fn; /* filenumber */
#endif

#ifdef __macosx
  struct anEnv env;
#endif

#ifdef SGI
  ALconfig config;
  ALport port;
#endif
  long rate;
  long gain;
  int output;
#ifdef ALSA
  void *handle;
  snd_pcm_format_t alsa_format;
#endif
  char *device;
  int channels;
  int format;
  int private1;
  void *private2;
#ifdef WIN32
  volume_control_handle *vh;
  int *refcnt; /* number of times audio device has been opened */
#else
  void *vh; /* Generic handle to mixer device */
#endif
};

struct audio_name {
  int  val;
  char *name;
  char *sname;
};


extern void audio_capabilities(struct audio_info_struct *);
extern void audio_fit_capabilities(struct audio_info_struct *ai,int c,int r);

extern char *audio_encoding_name(int format);

extern int audio_play_samples(struct audio_info_struct *,unsigned char *,int);
extern int audio_open(struct audio_info_struct *);
extern int audio_reset_parameters(struct audio_info_struct *);
extern int audio_rate_best_match(struct audio_info_struct *ai);
extern int audio_set_rate(struct audio_info_struct *);
extern int audio_set_format(struct audio_info_struct *);
extern int audio_get_formats(struct audio_info_struct *);
extern int audio_set_channels(struct audio_info_struct *);
extern int audio_write_sample(struct audio_info_struct *,short *,int);
extern int audio_close(struct audio_info_struct *);
extern void audio_info_struct_init(struct audio_info_struct *);
extern void audio_queueflush(struct audio_info_struct *ai);
extern int audio_set_gain(struct audio_info_struct *ai);
extern int audio_get_gain(struct audio_info_struct *ai);

