#include "portability.h"
#include "mpegindx.h"
#include "xing_header.h"
#include <stdio.h>
#include "pcmlevel.h"
#include "mp3time.h"
#include "segment.h"


typedef struct _pcmentry_t {
    int  pcmlevel;
    long pcmsum;
    long sec;
    long msec;
    long fpos;
#if 0
    struct _pcmentry_t *next;
#endif
} pcmentry_t;

#if 1 /* incomplete definition completed in segment.c and pcmlevel.c */
struct _mpgedit_pcmlevel_t
{
    int   stereo;
    int   silence_decibels;
    int   silence_threshold;
    int   silence_inflection_threshold;
    int   silence_hysteresis;
    int   silence_hysteresis_set;
    int   silence_repeat;
    int   minimum_inflection_time;
    int   minimum_time;
    int   silence_cnt;
    int   silence_print;

    long  silence_ssec;
    long  silence_smsec;
    long  silence_esec;
    long  silence_emsec;
    int   verbose;
    FILE  *pcmfp;
};
#endif



static char *g_progname;

void usage(void)
{
    printf("usage: %s [-v] [-c] [-d] [-m] mp3_file\n", g_progname);
    printf("          -c: silence repeat\n");
    printf("          -d: silence threshold (db down)\n");
    printf("          -m: time since last silence\n");
    exit(0);
}

#define MAX_PCM_SAMPLES 9


int integrate_first(mpgedit_pcmfile_t *fp,
                   pcmentry_t        *pcmlist,
                   int               pcmlist_len,
                   long              *ret_pcmsum)
{
    long pcmsum;
    int i;
    int go = 1;

    pcmsum = 0;
    for (i=0; go && i<MAX_PCM_SAMPLES; i++) {
        go = mpgedit_pcmlevel_read_entry(fp, &pcmlist[i].pcmlevel,
                                         &pcmlist[i].sec,
                                         &pcmlist[i].msec);
        if (go) {
            pcmlist[i].fpos = mpgedit_pcmlevel_tell(fp);
            pcmsum += pcmlist[i].pcmlevel;
        }
    }
    pcmlist[i-1].pcmsum = pcmsum;
    *ret_pcmsum = pcmsum;
    return go;
}


int integrate_next(mpgedit_pcmfile_t *fp,
                   pcmentry_t        *pcmlist,
                   int               pcmlist_len,
                   long              *ret_pcmsum)
{
    pcmentry_t newentry;
    long       newsum;
    int        go;

    go = mpgedit_pcmlevel_read_entry(fp, &newentry.pcmlevel,
                                         &newentry.sec,
                                         &newentry.msec);
    if (go) {
        newsum = pcmlist[MAX_PCM_SAMPLES-1].pcmsum -
                 pcmlist[0].pcmlevel + newentry.pcmlevel;
        memmove(&pcmlist[0],
                &pcmlist[1],
                sizeof(*pcmlist) * (MAX_PCM_SAMPLES-1));
        newentry.pcmsum = newsum;
        memcpy(&pcmlist[MAX_PCM_SAMPLES-1], &newentry, sizeof(newentry));
        *ret_pcmsum = newsum;
    }
    return go;
}




void integrate_samples(char *file, int decibels)
{
    mpgedit_pcmfile_t *fp;
    int                ver=0, pcmbits=0, secbits=0, msecbits=0;
    int                pcmavg, pcmmin, pcmmax;
    pcmentry_t         pcmlist[MAX_PCM_SAMPLES];
    long               pcmsum;
    long               integral_threshold;

    memset(pcmlist, 0, sizeof(pcmlist));
    fp = mpgedit_pcmlevel_open(file, "rb");
    if (!fp) {
        return;
    }
    mpgedit_pcmlevel_read_header(fp, &ver, &pcmbits, &secbits, &msecbits);
    if (ver!=1 || pcmbits!=16 || secbits!=22 || msecbits!=10) {
        return;
    }
    mpgedit_pcmlevel_read_average(fp, &pcmavg, &pcmmin, &pcmmax);
    integral_threshold = (pcmavg >> decibels) * MAX_PCM_SAMPLES;
    printf("pcmavg=%d threshold=%d integral_threshold=%ld\n",
           pcmavg, pcmavg >> decibels, integral_threshold);

    /*
     * Integrate the first sample
     */
    integrate_first(fp, pcmlist, MAX_PCM_SAMPLES, &pcmsum);
    printf("First integral=%ld\n", pcmsum);


    /*
     * Next integral is the current sum-first_value+next_value
     *     A' = A - S1 + Sn
     */
    while (integrate_next(fp, pcmlist, MAX_PCM_SAMPLES, &pcmsum)) {
        if (pcmsum < integral_threshold) {
            printf("t=%3ld:%02ld.%03ld A=%ld\n", 
                    pcmlist[MAX_PCM_SAMPLES-1].sec/60,
                    pcmlist[MAX_PCM_SAMPLES-1].sec%60,
                    pcmlist[MAX_PCM_SAMPLES-1].msec,
                    pcmsum);
        }
    }
}


/* Leading plus forces POSIX processing of argument list */
#define OPTARGS "+vd:m:c:IH:"

int main(int argc, char *argv[])
{
    char       *name;
    int        ch;
    int        v_flag = -1;
    mpgedit_pcmlevel_t levelconf;
    mpgedit_pcmfile_t *pcmh;
    mpgedit_pcmlevel_index_t *pcmindex;
    int        fpos;
    int        decibels = -1;
    int        minimum_time = -1;
    int        silence_repeat = -1;
    FILE       *sessionfp;
    mpeg_time  *tarray;
    int        tarraylen;
    int        indx;
    char       *path_base = "";
    char       *path_ext  = "";
    char       *path_dir;
    int        I_flag = 0;
    int        H_flag = 0;
    int        H_value = 0;
    int        pcmval;
    long       pcmsec;
    long       pcmmsec;
   

    g_progname = (g_progname = strrchr(argv[0], '/')) ? g_progname+1 : argv[0];

    if (argc == 1) {
        usage();
    }

    while ((ch = getopt(argc, argv, OPTARGS)) != -1) {
        switch (ch) {
          case 'c':
            silence_repeat = atoi(optarg);
            break;

          case 'd':
            decibels = atoi(optarg) / 6;
            break;

          case 'm':
            minimum_time = atoi(optarg);
            break;

          case 'v':
            v_flag = 1;
            break;

          case 'I':
            I_flag = 1;
            break;

          case 'H':
            H_flag = 1;
            H_value = atoi(optarg);
            break;

          case '?':
            fprintf(stderr, "unknown option '%c'\n", ch);
          case 'h':
            usage();
            break;
        }
    }

    memset(&levelconf, 0, sizeof(levelconf));
    levelconf.minimum_inflection_time = MINIMUM_INFLECTION_TIME;
    levelconf.verbose                 = v_flag > 0 ? v_flag : 0;
    levelconf.silence_decibels        = decibels > 0 ? decibels : SILENCE_30DB;
    levelconf.minimum_time            = minimum_time > 0 ? 
                                        minimum_time : MINIMUM_TIME;
    levelconf.silence_repeat          = silence_repeat > 0 ?
                                            silence_repeat : SILENCE_REPEAT;
    if (optind < argc) {
        name = argv[optind];
    }
    else {
        printf("ERROR: inputfile missing\n");
        usage();
    }

    sessionfp = fopen("levels_session", "wb");
    if (!sessionfp) {
       perror("open save session");
       return 1;
    }

    if (H_flag) {
        pcmh = mpgedit_pcmlevel_open(name, "rb");
        if (!pcmh) {
            perror("mpgedit_pcmlevel_open");
            return 0;
        }
        pcmindex = mpgedit_pcmlevel_generate_index(pcmh);
        fpos = mpgedit_pcmlevel_index_get_offset(pcmh, pcmindex, H_value);
        printf("fpos= %d\n", fpos);
        mpgedit_pcmlevel_seek(pcmh, fpos);
        mpgedit_pcmlevel_read_entry(pcmh,
                                    &pcmval, &pcmsec, &pcmmsec);
        printf("sec = %ld time=%ld:%02ld.%03ld\n", pcmsec, pcmsec/60,
                                                   pcmsec%60, pcmmsec);

        
        return 0;
    }

    if (I_flag) {
        integrate_samples(name, decibels);
        return 0;
    }

    /*
     * Input file name must not be the .lvl file.  Assume .mp3 here
     * This "correction" is needed so the correct file name is listed
     * in the levels_session file.
     */
    path_dir = mpgedit_pathname_parse(name, &path_base, &path_ext);
    if (!strcmp(path_ext, "lvl")) {
        name = (char *) malloc(strlen(path_dir) + strlen(path_base) + 
                               strlen(path_ext) + 3);
        if (name) {
            strcpy(name, path_dir);
            if (*path_base) {
                strcat(name, "/");
                strcat(name, path_base);
                strcat(name, ".mp3");
            }
        }
    }


    mpgedit_segment_find(name, &levelconf, &tarray, &tarraylen);
    for (indx = 1; indx < tarraylen; indx++) {
        fprintf(sessionfp, "%s%%%%%6ld:%02ld.%03ld%%%%%6ld:%02ld.%03ld\n",
                name,
                tarray[indx-1].units/60,
                tarray[indx-1].units%60,
                tarray[indx-1].usec/1000,

                tarray[indx].units/60,
                tarray[indx].units%60,
                tarray[indx].usec/1000);
    }
    fclose(sessionfp);

    return 0;
}
