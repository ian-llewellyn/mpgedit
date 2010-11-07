/*
 * Win32 Audio Mixer API
 *
 * Copyright (C) 2003-2006 Adam Bernstein
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307  USA.
 */

/*
 * Win32 dependent component of the Volume API.  The short story on how 
 * the Win32 mixer API works is there are destinations (like a speaker)
 * there are source lines (things that connect sound sources to
 * destinations), and controls (things on source lines that modify that
 * source line).  In principle, all you need to know is the numeric ID
 * (an integer value) of the control you want to twiddle, and you are done.
 * The trick is knowing that ID.  The majority of the work done by this
 * code is the search for the volume control on the WAVEOUT source
 * that is connected to the speakers.  Once that value is known, 
 * updating the volume is a simple matter.  See the main() test program
 * to see how all this code is called.  The top of the good stuff is
 * open_mixer_search_control(), a high level convenience routine to
 * lookup the ID for the specified destination, source connected to that 
 * destination and control on that source. Once the control ID is known,
 * setting the volume by calling set_control_details() is really easy.
 * 
 * This web site was extremely valuable to me when writing this code:
 *   http://www.borg.com/~jglatt/tech/mixer.htm
 *
 */
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>


int set_control_details(unsigned int device_id,
                        int control_id,
                        int control_value)
{

    MIXERCONTROLDETAILS_UNSIGNED value;
    MIXERCONTROLDETAILS          details;
    MMRESULT                     sts;
    HMIXER                       mixer;

    mixer = (HMIXER) device_id;

    value.dwValue       = control_value;
    details.dwControlID = control_id;
    details.cbStruct    = sizeof(MIXERCONTROLDETAILS);
    details.paDetails   = &value;
    details.cbDetails   = sizeof(MIXERCONTROLDETAILS_UNSIGNED);

    /* This is always 1 for a MIXERCONTROL_CONTROLF_UNIFORM control */
    details.cChannels = 1;

    /* This is always 0 except for a MIXERCONTROL_CONTROLF_MULTIPLE control */
    details.cMultipleItems = 0;

    sts = mixerSetControlDetails((HMIXEROBJ) mixer, 
                                  &details, MIXER_SETCONTROLDETAILSF_VALUE);
  
    return sts ? -1 : 0;
}


int get_control_details(unsigned int device_id, int control_id)
{
    MIXERCONTROLDETAILS_UNSIGNED value;
    MIXERCONTROLDETAILS          details;
    MMRESULT                     sts;
    HMIXER                       mixer;

    mixer = (HMIXER) device_id;

    details.dwControlID = control_id;
    details.cbStruct    = sizeof(MIXERCONTROLDETAILS);


    /* This is always 1 for a MIXERCONTROL_CONTROLF_UNIFORM control */
    details.cChannels   = 1;

    /* This is always 0 except for a MIXERCONTROL_CONTROLF_MULTIPLE control */
    details.cMultipleItems = 0;

    details.paDetails   = &value;
    details.cbDetails   = sizeof(MIXERCONTROLDETAILS_UNSIGNED);

    sts = mixerGetControlDetails((HMIXEROBJ) mixer,
                                  &details, MIXER_GETCONTROLDETAILSF_VALUE);
    return sts ? -1 : value.dwValue;
}


int get_mixer_controls(unsigned int device_id,
                       int cnt,
                       unsigned int control,
                       int control_id)
{
    MIXERCONTROL       control_array[20];
    MIXERLINECONTROLS  controls;
    MMRESULT           sts;
    int                i = 0;
    HMIXER             mixer;

    mixer = (HMIXER) device_id;

    controls.pamxctrl  = control_array;
    controls.cbmxctrl  = sizeof(MIXERCONTROL);
    controls.cControls = cnt;

    controls.cbStruct  = sizeof(MIXERLINECONTROLS);
    controls.dwLineID  = control_id;

    /* Retrieve info on all controls for this line simultaneously */
    sts = mixerGetLineControls((HMIXEROBJ) mixer, 
                               &controls, MIXER_GETLINECONTROLSF_ALL);
    if (sts) {
        return -1;
    }
    do {
        if (control_array[i].dwControlType == control) {
            return control_array[i].dwControlID;
        }
    } while (++i < cnt);
    return -1;
}


    
static int mixer_search_destination(HMIXER mixer, 
                                    int cnt, 
                                    unsigned int destination)
{
    MIXERLINE info;
    int       sts;
    int       i = 0;

    do {
        memset(&info, 0, sizeof(info));
        info.cbStruct      = sizeof(info);
        info.dwDestination = i;
        sts = mixerGetLineInfo((HMIXEROBJ) mixer,
                               &info, MIXER_GETLINEINFOF_DESTINATION);
        if (sts) {
            return -1;
        }
        if (info.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS) {
            return info.cConnections;
        }
    } while (++i < cnt);
    return -1;
}


static int mixer_search_source(HMIXER mixer,
                               int cnt,
                               unsigned intsource,
                               int *ret_line_id)
{
    MIXERLINE info;
    int       sts;
    int       i     = 0;

    do {
        memset(&info, 0, sizeof(info));
        info.cbStruct = sizeof(info);
        info.dwSource = i;
        sts = mixerGetLineInfo((HMIXEROBJ) mixer, &info, 
                                MIXER_GETLINEINFOF_SOURCE);
        if (sts) {
            return -11;
        }
        if (info.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT) {
            *ret_line_id = info.dwLineID;
            return info.cControls;
        }
    } while (++i < cnt);
    return -1;
}


char *mixer_strerror(int val)
{
    char *str;

    switch (val) {
      case -1:
        str = "ERROR: No audio mixer found";
        break;

      case -2:
        str = "Error opening mixer device";
        break;

      case -3:
        str = "Error getting mixer capabilities";
        break;

      case -4:
        str = "No speaker device found";
        break;

      case -5:
        str = "No PCM playback device found";
        break;

      case -6:
        str = "No volume control found for PCM device";
        break;

      default:
        str = "Unknown error code";
        break;
    }
    return str;
}

    
/*
 * This function opens the mixer device, then searches for the 
 * specified control.  This is accomplished by first searching for
 * a destination device, then a source connected to that destination,
 * and finally a control on the source.  Lots of work to accomplish such
 * a simple task.
 */
int open_mixer_search_control(unsigned int *device_id,
                              unsigned int destination,
                              unsigned int source,
                              unsigned int control)
{
    MIXERCAPS caps;
    int sts;
    int num_src;
    int num_controls;
    int source_id;
    int control_id;
    HMIXER *mixer;


    if (mixerGetNumDevs() <= 0) {
        return -1;
    }

    mixer = (HMIXER *) device_id;
    if (mixerOpen(mixer, 0, 0, 0, MIXER_OBJECTF_MIXER)) {
        return -2;
    }

    sts = mixerGetDevCaps((unsigned int) (*mixer), &caps, sizeof(caps));
    if (sts) {
        return -3;
    }
    num_src = mixer_search_destination(*mixer, caps.cDestinations,
                                       destination);
    if (num_src == -1) {
        return -4;
    }
    num_controls = mixer_search_source(*mixer, 
                                       num_src, 
                                       source,
                                       &source_id);

    if (num_controls == -1) {
        return -5;
    }
    control_id = get_mixer_controls((unsigned int) *mixer,
                                    num_controls,
                                    control,
                                    source_id);
    if (control_id == -1) {
        return -6;
    }
    return control_id;
}


void close_mixer_control(unsigned int device_id)
{
    mixerClose((HMIXER) device_id);
}


    
/*
 * device_id is the returned handle to the mixer device.  The return value of
 * this function is the control ID to the WAVEOUT volume control.  It may be
 * argued the position of these two parameters should be swapped, to be more
 * consistent with UNIX fd = open("/dev/mixer"); paradigm. However, his change
 * will percolate all the way thorough this API, and this top function is just
 * a convenience layer. At what point does the mapping to make it more UNIX-
 * like need to occur?  The caller of this function is my argument.
 */
int open_mixer_pcm_volume_control(unsigned int *device_id)
{
    return open_mixer_search_control(
               device_id,
               MIXERLINE_COMPONENTTYPE_DST_SPEAKERS,
               MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT,
               MIXERCONTROL_CONTROLTYPE_VOLUME);
}


#ifdef UNIT_TEST
int main(int argc, char *argv[])
{
    unsigned int mixer;
    int          control_id;
    int          volume;
    int          new_volume = 5000;

    if (argc > 1) {
        new_volume = atoi(argv[1]);
    }

    control_id = open_mixer_pcm_volume_control(&mixer);
    if (control_id < 0) {
        printf("%s\n", mixer_strerror(control_id));
        return 1;
    }

    /* 
     * Control ID is what we needed all along.  Just this value is
     * needed to reference the PCM out volume control for the speakers.
     */
    volume = get_control_details(mixer, control_id);
    if (volume != -1) {
        printf("get_control_details: control=%d value=%lu\n",
                control_id, volume);

        /*
         * Assume no further errors, as we could get volume from control.
         */
        set_control_details(mixer, control_id, new_volume);
        volume = get_control_details(mixer, control_id);
        printf("get_control_details: control=%d value=%lu\n",
                control_id, volume);
    }
    else {
        printf("Error getting volume from control\n");
    }
}

    return 0;
}
#endif
