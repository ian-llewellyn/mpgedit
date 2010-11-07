/*- This is a 80 chars line, to have pretty formatted comments ---------------*/

/* audio_macosx.c, originally written by Guillaume Outters
 * to contact the author, please mail to: guillaume.outters@free.fr
 * AudioUnit version by Steven A. Kortze <skortze@sourceforge.net>, reviewed by
 * Guillaume Outters
 *
 * This file is some quick pre-alpha patch to allow me to have some music for my
 * long working days, and it does it well. But it surely isn't a final version;
 * as Mac OS X requires at least a G3, I'm not sure it will be useful to
 * implement downsampling.
 *
 * Mac OS X audio works by asking you to fill its buffer, and, to complicate a
 * bit, you must provide it with floats. In order not to patch too much mpg123,
 * we'll accept signed short (mpg123 "approved" format) and transform them into
 * floats as soon as received. Let's say this way calculations are faster.
 *
 * As we don't have some /dev/audio device with blocking write, we'll have to
 * stop mpg123 before it does too much work, while we are waiting our dump proc
 * to be called. We'll uses semaphores for that, after some time using an awful
 * sleep() / kill() solution while waiting for Apple to implement semaphores in
 * the Public Beta (well, they were implemented, but that ENOSYS looked somewhat
 * unfriendly to me) (or perhaps for me to learn how to use semaphores in the
 * Beta).
 *
 * In order always to have a ready buffer to be dumped when the routine gets
 * called, we have a "buffer loop" of NUMBER_BUFFERS buffers. mpg123 fills it
 * on one extremity ('to'), playProc reads it on another point ('from'). 'to'
 * blocks when it arrives on 'from' (having filled the whole circle of buffers)
 * and 'from' blocks when no data is available. As soon as it has emptied a
 * buffer, if mpg123 is sleeping, it awakes it quite brutaly (SIGUSR2) to tell
 * it to fill the buffer. */

/* We can use either AudioUnits, or direct device addressing. */

#define _MIXER_SCALE_FACTOR 2.0
#ifndef MOSX_USES_AUDIOUNIT
#define MOSX_USES_AUDIOUNIT 1
#endif

/* Mac OS X introduces AudioUnits v.2, sufficently different from version 1 to
 * require code changes. Unfortunately I don't know how to detect if version 2
 * is present, and I don't have a 10.1 machine to test it the empirical way. */

#ifndef MOSX_AUDIOUNIT_V2
#define MOSX_AUDIOUNIT_V2 1
#endif

#include "mpg123.h"
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

/* Didn't I tell you I had some paranoid behaviours? */

#define G(troubleMaker,dest) if(err) { fprintf(stderr, "### %s: error %d\n", troubleMaker, (int)err); goto dest; }
#define DEB if(0) {
#define EB(x) x:
#define FEB }


#define ENV ((struct anEnv *)inClientData)
#define NUMBER_BUFFERS 2 	/* Tried with 3 buffers, but then any little window move is sufficient to stop the sound (OK, on a G3 400 with a Public Beta. Perhaps now we can go down to 2 buffers). With 16 buffers we have 1.5 seconds music buffered (or, if you're pessimistic, 1.5 seconds latency). Note 0 buffers don't work much further than the Bus error. */


void destroyBuffers(struct audio_info_struct *ai)
{
	aBuffer * ptr;
	aBuffer * ptr2;
	
	ptr = ai->env.to->next;
	ai->env.to->next = NULL;
	while(ptr)
	{
		ptr2 = ptr->next;
		if(ptr->buffer) free(ptr->buffer);
		free(ptr);
		ptr = ptr2;
	}
}


int initBuffers(struct audio_info_struct *ai)
{
	long m;
	aBuffer ** ptrptr;
	
	ptrptr = &ai->env.to;
	for(m = 0; m < NUMBER_BUFFERS; m++)
	{
		*ptrptr = malloc(sizeof(aBuffer));
		(*ptrptr)->size = 0;
		(*ptrptr)->remaining = 0;
		(*ptrptr)->buffer = NULL;
		ptrptr = &(*ptrptr)->next;
		sem_post(ai->env.semaphore);	/* This buffer is ready for filling (of course, it is empty!) */ 
	}
	*ptrptr = ai->env.from = ai->env.to;
	
	return 0; /* TO DO: error handling is a bit precarious here */
}


int OutputMixerGetVolume(AudioUnit unit)
{
    Float32 volume;
    int     sts;

    sts = AudioUnitGetParameter(
              unit,
              kHALOutputParam_Volume, kAudioUnitScope_Global,
              0, &volume);
    return sts ? -1 : (int) lroundf((volume * 100.0) / _MIXER_SCALE_FACTOR);
}


void OutputMixerSetVolume(AudioUnit unit, int volume)
{
    int sts;
    sts = AudioUnitSetParameter(unit,
                                kHALOutputParam_Volume, kAudioUnitScope_Global,
                                0, volume * 0.01 * _MIXER_SCALE_FACTOR, 0);
}


int fillBuffer(aBuffer * b, short * source, long size)
{
	float * dest;
	
	if(b->remaining)	/* Non empty buffer, must still be playing */
		return(-1);
	if(b->size != size) 	/* Hey! What's that? Coudn't this buffer size be fixed once (well, perhaps we just didn't allocate it yet) */
	{
		if(b->buffer) free(b->buffer);
		b->buffer = malloc(size * sizeof(float));
		b->size = size;
	}
	
	dest = b->buffer;
	while(size--)
		/* *dest++ = ((*source++) + 32768) / 65536.0; */
		*dest++ = (*source++) / 32768.0;
	
	b->ptr = b->buffer;
	b->remaining = b->size; /* Do this at last; we shouldn't show the buffer is full before it is effectively */
	
	#ifdef DEBUG_MOSX
	printf("."); fflush(stdout);
	#endif
	
	return(0);
}

#if MOSX_USES_AUDIOUNIT
#if MOSX_AUDIOUNIT_V2
OSStatus playProc(void * inClientData, AudioUnitRenderActionFlags * inActionFlags, const AudioTimeStamp * inTimeStamp, UInt32 inBusNumber, UInt32 inNumFrames, AudioBufferList * outOutputData)
#else
OSStatus playProc(void * inClientData, AudioUnitRenderActionFlags inActionFlags, const AudioTimeStamp * inTimeStamp, UInt32 inBusNumber, AudioBuffer * ioData)
#endif
#else
OSStatus playProc(AudioDeviceID inDevice, const AudioTimeStamp * inNow, const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime, AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData)
#endif
{
	#define BUFFER_LIST ( ( ! MOSX_USES_AUDIOUNIT ) || MOSX_AUDIOUNIT_V2 )
	
	long m, n;
	float * dest;
	
	#if BUFFER_LIST
	int o;
	
	for(o = 0; o < outOutputData->mNumberBuffers; ++o)
	#endif
	{
		/* What we have to fill */
		
		#if ! BUFFER_LIST
		m = ioData->mDataByteSize / sizeof(float);
		dest = ioData->mData;
		#else
		m = outOutputData->mBuffers[o].mDataByteSize / sizeof(float);
		dest = outOutputData->mBuffers[o].mData;
		#endif
		
		while(m > 0)
		{
			if( (n = ENV->from->remaining) <= 0 )   /* No more bytes in the current read buffer! */
			{
				while( (n = ENV->from->remaining) <= 0)
					usleep(2000);   /* Let's wait a bit for the results... */
			}
			
			/* We dump what we can */
			
			if(n > m) n = m;	/* In fact, just the necessary should be sufficient (I think) */
			
			memcpy(dest, ENV->from->ptr, n * sizeof(float));
			dest += n;
			
			/* Let's remember all done work */
			
			m -= n;
			ENV->from->ptr += n;
			if( (ENV->from->remaining -= n) <= 0)   /* ... and tell mpg123 there's a buffer to fill */
			{
				sem_post(ENV->semaphore);
				ENV->from = ENV->from->next;
			}
		}
	}
	
	return (0); 
}

int audio_open(struct audio_info_struct *ai)
{
	#if MOSX_USES_AUDIOUNIT
	#if MOSX_AUDIOUNIT_V2
	ComponentDescription desc;
	Component	comp;
	AURenderCallbackStruct inputCallback = { playProc, &ai->env };
	#else
	struct AudioUnitInputCallback inputCallback = { playProc, &ai->env };
	#endif
	#else
	long size;
	#endif
	OSStatus err;
	AudioStreamBasicDescription format;
	char s[10];
	long m;
	
	/* Where did that default audio output go? */
	
	#if MOSX_USES_AUDIOUNIT
	#if MOSX_AUDIOUNIT_V2
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple; /* NOTE: and if default output isn't Apple? */
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	if(!(comp = FindNextComponent(NULL, &desc))) { fprintf(stderr, "### No default audio output component\n"); goto e0; }
	err = OpenAComponent(comp, &ai->env.output); G("OpenAComponent()", e0)
	#else
	err = OpenDefaultAudioOutput(&ai->env.output); G("OpenDefaultAudioOutput()", e0)
	#endif /* MOSX_AUDIOUNIT_V2 */
	err = AudioUnitInitialize(ai->env.output); G("AudioUnitInitialize()", e1);
	#else
	size = sizeof(ai->env.output);
	err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &ai->env.output); G("AudioHardwareGetProperty(DefaultOutputDevice)", e0)
	#endif
	
	/* Let's init our environment */
	
	ai->env.size = 0;
	ai->env.debut = NULL;
	ai->env.ptr = NULL;
	ai->env.play = 0;
	
	/* Hmmm, let's choose PCM format */
	
	#if MOSX_USES_AUDIOUNIT
	/*
	 * We tell the Output Unit what format we're going to supply data to it.
	 * This is necessary if you're providing data through an input callback
	 * AND you want the DefaultOutputUnit to do any format conversions
	 * necessary from your format to the device's format.
	 */

	/* The sample rate of the audio stream */
	format.mSampleRate = ai->rate; 

	/* The specific encoding type of audio stream*/
	format.mFormatID = kAudioFormatLinearPCM; 

	/* flags specific to each format */
	format.mFormatFlags =
            kLinearPCMFormatFlagIsFloat |
            kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsPacked;

	/*
	 * We produce 2-channel audio. Now if we have a mega-super-hyper
	 * card for our audio, it is its problem to convert it to 8-, 16-,
	 * 32- or 1024-channel data.
	 */
	format.mFramesPerPacket  = 1;
	format.mChannelsPerFrame = ai->channels;
	format.mBytesPerPacket   = format.mChannelsPerFrame * sizeof(float);
	format.mBytesPerFrame    = format.mBytesPerPacket;

	/* One of the most constant constants of the whole computer history. */
	format.mBitsPerChannel = sizeof(float) * 8; 
	
	err = AudioUnitSetProperty(ai->env.output, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(AudioStreamBasicDescription)); G("AudioUnitSetProperty(StreamFormat)", e2)
	#else
	size = sizeof(format);
	err = AudioDeviceGetProperty(ai->env.output, 0, 0, kAudioDevicePropertyStreamFormat, &size, &format); G("AudioDeviceGetProperty(StreamFormat)", e0)
	if(format.mFormatID != kAudioFormatLinearPCM) err = kAudioDeviceUnsupportedFormatError; G("AudioDeviceGetProperty(StreamFormat)", e0)
	#endif
	
	/* A semaphore can be quite decorative, too. At least the "original"
	 * semaphore (released at about 0xC0 before Epoch). */
	
	strcpy(s, "/mpg123-0000");
	do
	{
		for(m = 10;; m--)
			if( (s[m]++) <= '9')
				break;
			else
				s[m] = '0';
	} while( (ai->env.semaphore = sem_open(s, O_CREAT | O_EXCL, 0644, 0)) == (sem_t *)SEM_FAILED );
	
	if(initBuffers(ai) < 0) goto e2;
	
	/* And prepare audio launching */
	
	#if MOSX_USES_AUDIOUNIT
	#if MOSX_AUDIOUNIT_V2
	err = AudioUnitSetProperty(ai->env.output, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &inputCallback, sizeof(inputCallback)); G("AudioUnitSetProperty(RenderCallback)", e3)
	#else
	err = AudioUnitSetProperty(ai->env.output, kAudioUnitProperty_SetInputCallback, kAudioUnitScope_Input, 0, &inputCallback, sizeof(inputCallback)); G("AudioUnitSetProperty(InputCallback)", e3)
	#endif
	#else
	err = AudioDeviceAddIOProc(ai->env.output, playProc, &ai->env); G("AudioDeviceAddIOProc", e3)
	#endif

	#if MOSX_USES_AUDIOUNIT
	#if MOSX_AUDIOUNIT_V2
        if (ai->gain>=0) {
            audio_set_gain(ai);
        }
	#endif
	#endif
	
	return(0);
	
	/* I dream of a programming language where you tell after each operation its
	 * reverse in case what follows fails. */
	
	DEB
	EB(e3)	destroyBuffers(ai);
	#if MOSX_USES_AUDIOUNIT
	EB(e2)	AudioUnitUninitialize(ai->env.output);
	EB(e1) CloseComponent(ai->env.output);
	#else
	EB(e2)
	#endif
	EB(e0) return(-1);
	FEB
}


int audio_set_gain(struct audio_info_struct *ai)
{
    int rvol;
    int lvol;
    int volume;

    if (!ai) {
        return -1;
    }
    
    rvol = ai->gain >> 8;
    lvol = (ai->gain & 0xff);


    volume = (rvol+lvol) / 2;
    OutputMixerSetVolume(ai->env.output, volume);
    return 0;
}


int audio_get_gain(struct audio_info_struct *ai)
{
    int volume;
    if (!ai) {
        return -1;
    }
    
    volume = OutputMixerGetVolume(ai->env.output);
    if (volume == -1) {
        return -1;
    }
    ai->gain = ((volume & 0xff) << 8) | (volume & 0xff);

    return 0;
}


int audio_reset_parameters(struct audio_info_struct *ai)
{
	return 0;
}

int audio_rate_best_match(struct audio_info_struct *ai)
{
	return 0;
}

int audio_set_rate(struct audio_info_struct *ai)
{
	return 0;
}

int audio_set_channels(struct audio_info_struct *ai)
{
	return 0;
}

int audio_set_format(struct audio_info_struct *ai)
{
	return 0;
}

int audio_get_formats(struct audio_info_struct *ai)
{
	return AUDIO_FORMAT_SIGNED_16;
}

int audio_play_samples(struct audio_info_struct *ai,unsigned char *buf,int len)
{
	/* We have to calm down mpg123, else he wouldn't hesitate to drop us another
	 * buffer (which would be the same, in fact) */
	
	while(sem_wait(ai->env.semaphore)){}	/* We just have to wait a buffer fill request */
	fillBuffer(ai->env.to, (short *)buf, len / sizeof(short));
	ai->env.to = ai->env.to->next;
	
	/* And we lauch action if not already done */
	
	if(!ai->env.play)
	{
		#if MOSX_USES_AUDIOUNIT
		if(AudioOutputUnitStart(ai->env.output)) return(-1);
		#else
		if(AudioDeviceStart(ai->env.output, playProc)) return(-1);
		#endif
		ai->env.play = 1;
	}
	
	return len;
}

int audio_close(struct audio_info_struct *ai)
{
	#if MOSX_USES_AUDIOUNIT
	AudioOutputUnitStop(ai->env.output);
	AudioUnitUninitialize(ai->env.output);
	CloseComponent(ai->env.output);
	#else
	AudioDeviceStop(ai->env.output, playProc);  /* No matter the error code, we want to close it (by brute force if necessary) */
	AudioDeviceRemoveIOProc(ai->env.output, playProc);
	#endif
	destroyBuffers(ai);
	sem_close(ai->env.semaphore);
	
	return(0); /* It is often appreciated that closing procedures minimize failure; those who break everything just before leaving are not exactly the kind of people you'll invite again. By returning 0 while blindly ignoring our subprocedures errors, we fake an overpolite guest, which we never tried to be. */
}
