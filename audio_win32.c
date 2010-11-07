
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include "mpg123.h"
#include "volumeif.h"

#include <windows.h>

static CRITICAL_SECTION        cs;

static HWAVEOUT dev    = NULL;
static int nBlocks             = 0;
static int MAX_BLOCKS  = 6;
static int g_refcnt = 0;

static _inline void wait(void)
{
   while(nBlocks)
       Sleep(77);
}

static void CALLBACK wave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
   WAVEHDR *wh;
   HGLOBAL hg;

   if(uMsg == WOM_DONE)
   {
       EnterCriticalSection( &cs );

       wh = (WAVEHDR *)dwParam1;

       waveOutUnprepareHeader(dev, wh, sizeof (WAVEHDR));

       //Deallocate the buffer memory
       hg = GlobalHandle(wh->lpData);
       GlobalUnlock(hg);
       GlobalFree(hg);

       //Deallocate the header memory
       hg = GlobalHandle(wh);
       GlobalUnlock(hg);
       GlobalFree(hg);

       // decrease the number of USED blocks
       nBlocks--;

       LeaveCriticalSection( &cs );
   }
}

int audio_open(struct audio_info_struct *ai)
{
   MMRESULT res;
   WAVEFORMATEX outFormatex;

   if(ai->rate == -1)
       return(0);

   if (!ai->refcnt) {
       ai->refcnt = &g_refcnt;
   }
   if(!waveOutGetNumDevs())
   {
       MessageBox(NULL, "No audio devices present!", "Error...", MB_OK);
       return -1;
   }

   outFormatex.wFormatTag      = WAVE_FORMAT_PCM;
   outFormatex.wBitsPerSample  = 16;
   outFormatex.nChannels       = (ai->channels == 1 || ai->channels == 2) ?
                                     ai->channels : 2;
   /*
    * WaveOut does not handle 11025/mono PCM data correctly.  This hack
    * sets the data rate to 22050, but sets a private data value for
    * the data rate in the audio_info_struct to -11025.  This value
    * is interpreted by audio_play_samples() to double the data samples
    * present, so playback is at the correct speed.
    */
   if (ai->channels == 1 && ai->rate == 11025) {
      outFormatex.nSamplesPerSec  = 22050;
      ai->rate = -11025;
   }
   else {
      outFormatex.nSamplesPerSec  = ai->rate;
   }
   outFormatex.nAvgBytesPerSec = outFormatex.nSamplesPerSec * outFormatex.nChannels * outFormatex.wBitsPerSample/8;
   outFormatex.nBlockAlign     = outFormatex.nChannels * outFormatex.wBitsPerSample/8;

   res = waveOutOpen(&dev, (UINT)ai->device, &outFormatex, (DWORD)wave_callback, 0, CALLBACK_FUNCTION);

   if(res != MMSYSERR_NOERROR)
   {
       switch(res)
       {
           case MMSYSERR_ALLOCATED:
               MessageBox(NULL, "Device Is Already Open", "Error...", MB_OK);
               break;
           case MMSYSERR_BADDEVICEID:
               MessageBox(NULL, "The Specified Device Is out of range", "Error...", MB_OK);
               break;
           case MMSYSERR_NODRIVER:
               MessageBox(NULL, "There is no audio driver in this system.", "Error...", MB_OK);
               break;
           case MMSYSERR_NOMEM:
              MessageBox(NULL, "Unable to allocate sound memory.", "Error...", MB_OK);
               break;
           case WAVERR_BADFORMAT:
               MessageBox(NULL, "This audio format is not supported.", "Error...", MB_OK);
               break;
           case WAVERR_SYNC:
               MessageBox(NULL, "The device is synchronous.", "Error...", MB_OK);
               break;
           default:
               MessageBox(NULL, "Unknown Media Error", "Error...", MB_OK);
               break;
       }
       return -1;
   }

   (*ai->refcnt)++;
   ai->vh = mpgedit_volume_open();
   audio_set_gain(ai);

   waveOutReset(dev);
   InitializeCriticalSection(&cs);

   return 0;
}


int audio_set_gain(struct audio_info_struct *ai)
{
    int lvol = -1;
    int rvol = -1;

    if (!ai) {
        return -1;
    }

    if (ai->gain != -1) {
        lvol = ai->gain  & 0xff;
        rvol = (ai->gain & 0xff) >> 8;
        mpgedit_volume_set(ai->vh, lvol, rvol);
    }

    return 0;
}


int audio_get_gain(struct audio_info_struct *ai)
{
    int lvol = 0;
    int rvol = 0;
    int sts;

    if (!ai) {
        return -1;
    }

    sts = mpgedit_volume_get(ai->vh, &lvol, &rvol);
    if (sts != -1) {
        ai->gain = ((rvol & 0xff) << 8) | (lvol & 0xff);
    }
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
   HGLOBAL hg, hg2;
   LPWAVEHDR wh;
   MMRESULT res;
   void *b;

   ///////////////////////////////////////////////////////
   //  Wait for a few FREE blocks...
   ///////////////////////////////////////////////////////
   while(nBlocks > MAX_BLOCKS)
       Sleep(77);

   ////////////////////////////////////////////////////////
   // FIRST allocate some memory for a copy of the buffer!
   ////////////////////////////////////////////////////////
   hg2 = GlobalAlloc(GMEM_MOVEABLE, len*2);
   if(!hg2)
   {
       MessageBox(NULL, "GlobalAlloc failed!", "Error...",  MB_OK);
       return(-1);
   }
   b = GlobalLock(hg2);


   //////////////////////////////////////////////////////////
   // Here we can call any modification output functions we want....
   ///////////////////////////////////////////////////////////
   /* Double each sample, if 11025/mono */
   if (ai->channels == 1 && ai->rate == -11025) {
      unsigned short *src  = (unsigned short *) buf;
      unsigned short *dest = (unsigned short *) b;
      unsigned short *end  = (unsigned short *) (buf + len);
      while (src < end) {
          *dest++ = *src;
          *dest++ = *src++;
      }
      len *= 2;
   }
   else {
       CopyMemory(b, buf, len);
   }

   ///////////////////////////////////////////////////////////
   // now make a header and WRITE IT!
   ///////////////////////////////////////////////////////////
   hg = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (WAVEHDR));
   if(!hg)
   {
       return -1;
   }
   wh = GlobalLock(hg);
   wh->dwBufferLength = len;
   wh->lpData = b;


   EnterCriticalSection( &cs );

   res = waveOutPrepareHeader(dev, wh, sizeof (WAVEHDR));
   if(res)
   {
       GlobalUnlock(hg);
       GlobalFree(hg);
       LeaveCriticalSection( &cs );
       return -1;
   }

   res = waveOutWrite(dev, wh, sizeof (WAVEHDR));
   if(res)
   {
       GlobalUnlock(hg);
       GlobalFree(hg);
       LeaveCriticalSection( &cs );
       return (-1);
   }

   nBlocks++;

   LeaveCriticalSection( &cs );

   return(len);
}

int audio_close(struct audio_info_struct *ai)
{
   if (ai->vh) {
       mpgedit_volume_close(ai->vh);
       ai->vh = NULL;
   }

   (*ai->refcnt)--;
   if (*ai->refcnt > 0) {
       return(0);
   }
       
   if(dev)
   {
       wait();

       waveOutReset(dev);      //reset the device
       waveOutClose(dev);      //close the device
       dev=NULL;
   }

   DeleteCriticalSection(&cs);

   nBlocks = 0;
   return(0);
}
