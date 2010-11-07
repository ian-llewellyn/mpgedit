#ifndef _WMIXER_H
#define _WMIXER_H

/* 65535 is maximum integer value for top volume; to us 100 is top volume */
#define WIN32_MIXER_SCALE_FACTOR 655

int set_control_details(unsigned int device_id,
                        int control_id, int control_value);

int get_control_details(unsigned int device_id, int control_id);

int get_mixer_controls(unsigned int device_id, int cnt,
                       int control, int control_id) ;

int open_mixer_search_control(unsigned int *device_id,
                              unsigned int destination,
                              unsigned int source,
                              unsigned int control);
int open_mixer_pcm_volume_control(unsigned int *device_id);

void close_mixer_control(unsigned int device_id);

char *mixer_strerror(int val);



#endif
