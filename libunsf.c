#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libunsf.h"

#ifndef TRUE
#define TRUE         -1
#define FALSE        0
#endif

#undef MIN
#undef MAX
#undef MID

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))

#define LINKED_SAMPLE 8
#define LEFT_SAMPLE 4
#define RIGHT_SAMPLE 2
#define MONO_SAMPLE 1

static int err = 0;

typedef struct VelocityRangeList
{
    int range_count;
    unsigned char velmin[128];
    unsigned char velmax[128];
    unsigned char mono_patches[128];
    unsigned char left_patches[128];
    unsigned char right_patches[128];
    unsigned char other_patches[128];
} VelocityRangeList;

/* manually set the velocity of either a instrument or drum since most
   applications do not know about the extended patch format. */
static signed char melody_velocity_override[128][128];
static signed char drum_velocity_override[128][128];

static int tonebank[128];
static char *tonebank_name[128];
static char *voice_name[128][128];
static int voice_samples_mono[128][128];
static int voice_samples_left[128][128];
static int voice_samples_right[128][128];
static VelocityRangeList *voice_velocity[128][128];
static char *drumset_name[128];
static char *drumset_short_name[128];
static char *drum_name[128][128];
static int drum_samples_mono[128][128];
static int drum_samples_left[128][128];
static int drum_samples_right[128][128];
static VelocityRangeList *drum_velocity[128][128];
static char basename[80];
static char cpyrt[256];

static FILE *cfg_fd;

/* SoundFont chunk format and ID values */
typedef struct RIFF_CHUNK
{
    int size;
    int id;
    int type;
    int end;
} RIFF_CHUNK;

/* SoundFont preset headers */
typedef struct sfPresetHeader
{
    char achPresetName[20];
    unsigned short wPreset;
    unsigned short wBank;
    unsigned short wPresetBagNdx;
    unsigned long dwLibrary;
    unsigned long dwGenre;
    unsigned long dwMorphology;
} sfPresetHeader;

/* SoundFont preset indexes */
typedef struct sfPresetBag
{
    unsigned short wGenNdx;
    unsigned short wModNdx;
} sfPresetBag;

/* SoundFont preset generators */
typedef struct rangesType
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned char byLo;
    unsigned char byHi;
#else
    unsigned char byHi;
   unsigned char byLo;
#endif
} rangesType;


typedef union genAmountType
{
    rangesType ranges;
    short shAmount;
    unsigned short wAmount;
} genAmountType;


typedef struct sfGenList
{
    unsigned short sfGenOper;
    genAmountType genAmount;
} sfGenList;

/* SoundFont instrument headers */
typedef struct sfInst
{
    char achInstName[20];
    unsigned short wInstBagNdx;
} sfInst;

/* SoundFont instrument indexes */
typedef struct sfInstBag
{
    unsigned short wInstGenNdx;
    unsigned short wInstModNdx;
} sfInstBag;

/* SoundFont sample headers */
typedef struct sfSample
{
    char achSampleName[20];
    unsigned long dwStart;
    unsigned long dwEnd;
    unsigned long dwStartloop;
    unsigned long dwEndloop;
    unsigned long dwSampleRate;
    unsigned char byOriginalKey;
    signed char chCorrection;
    unsigned short wSampleLink;
    unsigned short sfSampleType;		/* 1 mono,2 right,4 left,linked 8,0x8000=ROM */
} sfSample;

/* list of the layers waiting to be dealt with */
typedef struct EMPTY_WHITE_ROOM
{
    sfSample *sample;
    sfGenList *igen;
    sfGenList *pgen;
    sfGenList *global_izone;
    sfGenList *global_pzone;
    int igen_count;
    int pgen_count;
    int global_izone_count;
    int global_pzone_count;
    float volume;
    int stereo_mode;
} EMPTY_WHITE_ROOM;

#define MAX_WAITING  256


#define CID(a,b,c,d)    (((d)<<24)+((c)<<16)+((b)<<8)+((a)))
#define CID_RIFF  CID('R','I','F','F')
#define CID_LIST  CID('L','I','S','T')
#define CID_INFO  CID('I','N','F','O')
#define CID_sdta  CID('s','d','t','a')
#define CID_snam  CID('s','n','a','m')
#define CID_smpl  CID('s','m','p','l')
#define CID_pdta  CID('p','d','t','a')
#define CID_phdr  CID('p','h','d','r')
#define CID_pbag  CID('p','b','a','g')
#define CID_pmod  CID('p','m','o','d')
#define CID_pgen  CID('p','g','e','n')
#define CID_inst  CID('i','n','s','t')
#define CID_ibag  CID('i','b','a','g')
#define CID_imod  CID('i','m','o','d')
#define CID_igen  CID('i','g','e','n')
#define CID_shdr  CID('s','h','d','r')
#define CID_ifil  CID('i','f','i','l')
#define CID_isng  CID('i','s','n','g')
#define CID_irom  CID('i','r','o','m')
#define CID_iver  CID('i','v','e','r')
#define CID_INAM  CID('I','N','A','M')
#define CID_IPRD  CID('I','P','R','D')
#define CID_ICOP  CID('I','C','O','P')
#define CID_sfbk  CID('s','f','b','k')
#define CID_ICRD  CID('I','C','R','D')
#define CID_IENG  CID('I','E','N','G')
#define CID_ICMT  CID('I','C','M','T')
#define CID_ISFT  CID('I','S','F','T')



/* SoundFont generator types */
#define SFGEN_startAddrsOffset         0
#define SFGEN_endAddrsOffset           1
#define SFGEN_startloopAddrsOffset     2
#define SFGEN_endloopAddrsOffset       3
#define SFGEN_startAddrsCoarseOffset   4
#define SFGEN_modLfoToPitch            5
#define SFGEN_vibLfoToPitch            6
#define SFGEN_modEnvToPitch            7
#define SFGEN_initialFilterFc          8
#define SFGEN_initialFilterQ           9
#define SFGEN_modLfoToFilterFc         10
#define SFGEN_modEnvToFilterFc         11
#define SFGEN_endAddrsCoarseOffset     12
#define SFGEN_modLfoToVolume           13
#define SFGEN_unused1                  14
#define SFGEN_chorusEffectsSend        15
#define SFGEN_reverbEffectsSend        16
#define SFGEN_pan                      17
#define SFGEN_unused2                  18
#define SFGEN_unused3                  19
#define SFGEN_unused4                  20
#define SFGEN_delayModLFO              21
#define SFGEN_freqModLFO               22
#define SFGEN_delayVibLFO              23
#define SFGEN_freqVibLFO               24
#define SFGEN_delayModEnv              25
#define SFGEN_attackModEnv             26
#define SFGEN_holdModEnv               27
#define SFGEN_decayModEnv              28
#define SFGEN_sustainModEnv            29
#define SFGEN_releaseModEnv            30
#define SFGEN_keynumToModEnvHold       31
#define SFGEN_keynumToModEnvDecay      32
#define SFGEN_delayVolEnv              33
#define SFGEN_attackVolEnv             34
#define SFGEN_holdVolEnv               35
#define SFGEN_decayVolEnv              36
#define SFGEN_sustainVolEnv            37
#define SFGEN_releaseVolEnv            38
#define SFGEN_keynumToVolEnvHold       39
#define SFGEN_keynumToVolEnvDecay      40
#define SFGEN_instrument               41
#define SFGEN_reserved1                42
#define SFGEN_keyRange                 43
#define SFGEN_velRange                 44
#define SFGEN_startloopAddrsCoarse     45
#define SFGEN_keynum                   46
#define SFGEN_velocity                 47
#define SFGEN_initialAttenuation       48
#define SFGEN_reserved2                49
#define SFGEN_endloopAddrsCoarse       50
#define SFGEN_coarseTune               51
#define SFGEN_fineTune                 52
#define SFGEN_sampleID                 53
#define SFGEN_sampleModes              54
#define SFGEN_reserved3                55
#define SFGEN_scaleTuning              56
#define SFGEN_exclusiveClass           57
#define SFGEN_overridingRootKey        58
#define SFGEN_unused5                  59
#define SFGEN_endOper                  60


/*================================================================
 *	SoundFont layer structure
 *================================================================*/

enum {
    SF_startAddrs,		/* 0 sample start address -4 (0to*0xffffff)*/
    SF_endAddrs,		/* 1 */
    SF_startloopAddrs,	/* 2 loop start address -4 (0 to * 0xffffff) */
    SF_endloopAddrs,	/* 3 loop end address -3 (0 to * 0xffffff) */
    SF_startAddrsHi,	/* 4 high word of startAddrs */
    SF_lfo1ToPitch,		/* 5 main fm: lfo1-> pitch */
    SF_lfo2ToPitch,		/* 6 aux fm:  lfo2-> pitch */
    SF_env1ToPitch,		/* 7 pitch env: env1(aux)-> pitch */
    SF_initialFilterFc,	/* 8 initial filter cutoff */
    SF_initialFilterQ,	/* 9 filter Q */
    SF_lfo1ToFilterFc,	/* 10 filter modulation: lfo1->filter*cutoff */
    SF_env1ToFilterFc,	/* 11 filter env: env1(aux)->filter * cutoff */
    SF_endAddrsHi,		/* 12 high word of endAddrs */
    SF_lfo1ToVolume,	/* 13 tremolo: lfo1-> volume */
    SF_env2ToVolume,	/* 14 Env2Depth: env2-> volume */
    SF_chorusEffectsSend,	/* 15 chorus */
    SF_reverbEffectsSend,	/* 16 reverb */
    SF_panEffectsSend,	/* 17 pan */
    SF_auxEffectsSend,	/* 18 pan auxdata (internal) */
    SF_sampleVolume,	/* 19 used internally */
    SF_unused3,		/* 20 */
    SF_delayLfo1,		/* 21 delay 0x8000-n*(725us) */
    SF_freqLfo1,		/* 22 frequency */
    SF_delayLfo2,		/* 23 delay 0x8000-n*(725us) */
    SF_freqLfo2,		/* 24 frequency */
    SF_delayEnv1,		/* 25 delay 0x8000 - n(725us) */
    SF_attackEnv1,		/* 26 attack */
    SF_holdEnv1,		/* 27 hold */
    SF_decayEnv1,		/* 28 decay */
    SF_sustainEnv1,		/* 29 sustain */
    SF_releaseEnv1,		/* 30 release */
    SF_autoHoldEnv1,	/* 31 */
    SF_autoDecayEnv1,	/* 32 */
    SF_delayEnv2,		/* 33 delay 0x8000 - n(725us) */
    SF_attackEnv2,		/* 34 attack */
    SF_holdEnv2,		/* 35 hold */
    SF_decayEnv2,		/* 36 decay */
    SF_sustainEnv2,		/* 37 sustain */
    SF_releaseEnv2,		/* 38 release */
    SF_autoHoldEnv2,	/* 39 */
    SF_autoDecayEnv2,	/* 40 */
    SF_instrument,		/* 41 */
    SF_nop,			/* 42 */
    SF_keyRange,		/* 43 */
    SF_velRange,		/* 44 */
    SF_startloopAddrsHi,	/* 45 high word of startloopAddrs */
    SF_keynum,		/* 46 */
    SF_velocity,		/* 47 */
    SF_initAtten,		/* 48 */
    SF_keyTuning,		/* 49 */
    SF_endloopAddrsHi,	/* 50 high word of endloopAddrs */
    SF_coarseTune,		/* 51 */
    SF_fineTune,		/* 52 */
    SF_sampleId,		/* 53 */
    SF_sampleFlags,		/* 54 */
    SF_samplePitch,		/* 55 SF1 only */
    SF_scaleTuning,		/* 56 */
    SF_keyExclusiveClass,	/* 57 */
    SF_rootKey,		/* 58 */
    SF_EOF			/* 59 */
};


/* error handling macro */
#define BAD_SF()                                            \
{                                                           \
   fprintf(stderr, "Error: bad SoundFont structure\n");     \
   err = 1;                                                 \
   goto getout;                                             \
}

/* reads a byte from the input file */
static int get8(FILE *f)
{
    return getc(f);
}



/* reads a word from the input file (little endian) */
static int get16(FILE *f)
{
    int b1, b2;

    b1 = get8(f);
    b2 = get8(f);

    return ((b2 << 8) | b1);
}



/* reads a long from the input file (little endian) */
static int get32(FILE *f)
{
    int b1, b2, b3, b4;

    b1 = get8(f);
    b2 = get8(f);
    b3 = get8(f);
    b4 = get8(f);

    return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}



/* calculates the file offset for the end of a chunk */
static void calc_end(RIFF_CHUNK *chunk, FILE *f)
{
    chunk->end = ftell(f) + chunk->size + (chunk->size & 1);
}


/* reads and displays a SoundFont text/copyright message */
static void print_sf_string(FILE *f, char *title, int opt_no_write)
{
    char buf[256];
    char ch;
    int i = 0;

    do {
        ch = get8(f);
        buf[i++] = ch;
    } while ((ch) && (i < 256));

    if (i & 1)
        get8(f);

    if (!strncmp(title, "Made", 4)) {
        strcpy(cpyrt, "Made by ");
        strcat(cpyrt, buf);
    }

    if (!opt_no_write) fprintf(cfg_fd, "# %-12s%s\n", title, buf);
}


/* gets facts and names */
static int grab_soundfont_banks(UnSF_Options options, int sf_num_presets, sfPresetHeader *sf_presets,
                                sfPresetBag * sf_preset_indexes, sfGenList *sf_preset_generators,
                                sfInst *sf_instruments, sfInstBag *sf_instrument_indexes,
                                sfGenList * sf_instrument_generators, sfSample * sf_samples
)
{
    sfPresetHeader *pheader;
    sfPresetBag *pindex;
    sfGenList *pgen;
    sfInst *iheader;
    sfInstBag *iindex;
    sfGenList *igen;
    sfSample *sample;
    int pindex_count;
    int pgen_count;
    int iindex_count;
    int igen_count;
    int pnum, inum, jnum, lnum, num, drum;
    int wanted_patch, wanted_bank;
    int keymin, keymax, drumnum;
    int velmin, velmax;
    int i, j;
    char *s;
    char tmpname[80];

    for (i = 0; i < 128; i++) {
        tonebank[i] = FALSE;
        drumset_name[i] = NULL;
        drumset_short_name[i] = NULL;
        for (j = 0; j < 128; j++) {
            voice_name[i][j] = NULL;
            voice_samples_mono[i][j] = 0;
            voice_samples_left[i][j] = 0;
            voice_samples_right[i][j] = 0;
            voice_velocity[i][j] = NULL;
            drum_name[i][j] = NULL;
            drum_samples_mono[i][j] = 0;
            drum_samples_left[i][j] = 0;
            drum_samples_right[i][j] = 0;
            drum_velocity[i][j] = NULL;
        }
    }

    keymin = 0;
    keymax = 127;

    /* search for the desired preset */
    for (pnum=0; pnum<sf_num_presets; pnum++) {
        int global_preset_layer, global_preset_velmin, global_preset_velmax, preset_velmin, preset_velmax;
        int global_preset_keymin, global_preset_keymax, preset_keymin, preset_keymax;

        pheader = &sf_presets[pnum];


        wanted_patch = pheader->wPreset;
        wanted_bank = pheader->wBank;

        if (wanted_bank == 128 || options.opt_drum) {
            drum = TRUE;
            options.opt_drum_bank = wanted_patch;
        }
        else {
            drum = FALSE;
            options.opt_bank = wanted_bank;
        }

        /* find what substructures it uses */
        pindex = &sf_preset_indexes[pheader->wPresetBagNdx];
        pindex_count = pheader[1].wPresetBagNdx - pheader[0].wPresetBagNdx;

        if (pindex_count < 1)
            continue;

        /* prettify the preset name */
        s = getname(pheader->achPresetName);

        if (drum) {
            if (!drumset_name[options.opt_drum_bank]) {
                drumset_short_name[options.opt_drum_bank] = strdup(s);
                sprintf(tmpname, "%s-%s", basename, s);
                drumset_name[options.opt_drum_bank] = strdup(tmpname);
                if (options.opt_verbose) printf("drumset #%d %s\n", options.opt_drum_bank, s);
            }
        }
        else {
            if (!voice_name[options.opt_bank][wanted_patch]) {
                voice_name[options.opt_bank][wanted_patch] = strdup(s);
                if (options.opt_verbose) printf("bank #%d voice #%d %s\n", options.opt_bank, wanted_patch, s);
                tonebank[options.opt_bank] = TRUE;
            }
        }

        global_preset_velmin = preset_velmin = -1;
        global_preset_velmax = preset_velmax = -1;
        global_preset_keymin = preset_keymin = -1;
        global_preset_keymax = preset_keymax = -1;

        /* for each layer in this preset */
        for (inum=0; inum<pindex_count; inum++) {
            int global_instrument_layer, global_instrument_velmin, global_instrument_velmax,
                    instrument_velmin, instrument_velmax;
            int global_instrument_keymin, global_instrument_keymax,
                    instrument_keymin, instrument_keymax;

            pgen = &sf_preset_generators[pindex[inum].wGenNdx];
            pgen_count = pindex[inum+1].wGenNdx - pindex[inum].wGenNdx;

            if (pgen_count > 0 && pgen[pgen_count-1].sfGenOper != SFGEN_instrument)
                global_preset_layer = TRUE;
            else global_preset_layer = FALSE;

            if (global_preset_velmin >= 0) preset_velmin = global_preset_velmin;
            if (global_preset_velmax >= 0) preset_velmax = global_preset_velmax;
            if (global_preset_keymin >= 0) preset_keymin = global_preset_keymin;
            if (global_preset_keymax >= 0) preset_keymax = global_preset_keymax;

            if (pgen_count < 0) break;

            if (pgen[0].sfGenOper == SFGEN_keyRange) {
                preset_keymin = pgen[0].genAmount.ranges.byLo;
                preset_keymax = pgen[0].genAmount.ranges.byHi;
                if (global_preset_layer) {
                    global_preset_keymin = preset_keymin;
                    global_preset_keymax = preset_keymax;
                }
            }

            for (jnum=0; jnum<pgen_count; jnum++) {
                if (pgen[jnum].sfGenOper == SFGEN_velRange) {
                    preset_velmin = pgen[jnum].genAmount.ranges.byLo;
                    preset_velmax = pgen[jnum].genAmount.ranges.byHi;
                    if (global_preset_layer) {
                        global_preset_velmin = preset_velmin;
                        global_preset_velmax = preset_velmax;
                    }
                }
            }

            /* find what instrument we should use */
            if ((pgen_count > 0) &&
                (pgen[pgen_count-1].sfGenOper == SFGEN_instrument)) {

                iheader = &sf_instruments[pgen[pgen_count-1].genAmount.wAmount];

                iindex = &sf_instrument_indexes[iheader->wInstBagNdx];
                iindex_count = iheader[1].wInstBagNdx - iheader[0].wInstBagNdx;

                global_instrument_velmin = instrument_velmin = -1;
                global_instrument_velmax = instrument_velmax = -1;
                global_instrument_keymin = instrument_keymin = -1;
                global_instrument_keymax = instrument_keymax = -1;


                /* for each layer in this instrument */
                for (lnum=0; lnum<iindex_count; lnum++) {
                    igen = &sf_instrument_generators[iindex[lnum].wInstGenNdx];
                    igen_count = iindex[lnum+1].wInstGenNdx - iindex[lnum].wInstGenNdx;

                    if (global_instrument_velmin >= 0) instrument_velmin = global_instrument_velmin;
                    if (global_instrument_velmax >= 0) instrument_velmax = global_instrument_velmax;
                    if (global_instrument_keymin >= 0) instrument_keymin = global_instrument_keymin;
                    if (global_instrument_keymax >= 0) instrument_keymax = global_instrument_keymax;

                    if ((igen_count > 0) &&
                        (igen[igen_count-1].sfGenOper != SFGEN_sampleID))
                        global_instrument_layer = TRUE;
                    else global_instrument_layer = FALSE;


                    for (jnum=0; jnum<igen_count; jnum++) {
                        if (igen[jnum].sfGenOper == SFGEN_velRange) {
                            instrument_velmax = igen[jnum].genAmount.ranges.byHi;
                            instrument_velmin = igen[jnum].genAmount.ranges.byLo;
                            if (global_instrument_layer) {
                                global_instrument_velmin = instrument_velmin;
                                global_instrument_velmax = instrument_velmax;
                            }
                        }
                    }

                    if (igen_count > 0 && igen[0].sfGenOper == SFGEN_keyRange) {
                        instrument_keymax = igen[0].genAmount.ranges.byHi;
                        instrument_keymin = igen[0].genAmount.ranges.byLo;
                        if (global_instrument_layer) {
                            global_instrument_keymin = instrument_keymin;
                            global_instrument_keymax = instrument_keymax;
                        }
                    }


                    if (instrument_velmin >= 0) velmin = instrument_velmin;
                    else velmin = 0;
                    if (instrument_velmax >= 0) velmax = instrument_velmax;
                    else velmax = 127;
                    if (preset_velmin >= 0 && preset_velmax >= 0) {
                        if (preset_velmin >= velmin && preset_velmax <= velmax) {
                            velmin = preset_velmin;
                            velmax = preset_velmax;
                        }
                    }

                    if (instrument_keymin >= 0) keymin = instrument_keymin;
                    else keymin = 0;
                    if (instrument_keymax >= 0) keymax = instrument_keymax;
                    else keymax = 127;
                    if (preset_keymin >= 0 && preset_keymax >= 0) {
                        if (preset_keymin >= keymin && preset_keymax <= keymax) {
                            keymin = preset_keymin;
                            keymax = preset_keymax;
                        }
                    }

                    drumnum = keymin;

                    /* find what sample we should use */
                    if ((igen_count > 0) &&
                        (igen[igen_count-1].sfGenOper == SFGEN_sampleID)) {
                        int i;

                        sample = &sf_samples[igen[igen_count-1].genAmount.wAmount];

                        /* sample->wSampleLink is the link?? */
                        /* lsample = &sf_samples[sample->wSampleLink] */
                        if (sample->sfSampleType & LINKED_SAMPLE) {
                            printf("linked sample: link is %d\n", sample->wSampleLink);
                        }

                        if (sample->sfSampleType & LINKED_SAMPLE) continue; /* linked */

                        s = sample->achSampleName;
                        i = strlen(s)-1;

                        if (s[i] == 'L') sample->sfSampleType = LEFT_SAMPLE;
                        if (s[i] == 'R') sample->sfSampleType = RIGHT_SAMPLE;

                        /* prettify the sample name */
                        s = getname(sample->achSampleName);

                        if (sample->sfSampleType & 0x8000) {
                            printf("This SoundFont uses AWE32 ROM data in sample %s\n", s);
                            if (options.opt_veryverbose)
                                printf("\n");
                            continue;
                        }

                        if (drum) {
                            int pool_num;
                            for (pool_num = keymin; pool_num <= keymax; pool_num++) {
                                drumnum = pool_num;
                                if (!drum_name[options.opt_drum_bank][drumnum]) {
                                    drum_name[options.opt_drum_bank][drumnum] = strdup(s);
                                    if (options.opt_verbose) printf("drumset #%d drum #%d %s\n", options.opt_drum_bank, drumnum, s);
                                }
                                if (sample->sfSampleType == LEFT_SAMPLE)
                                    drum_samples_left[options.opt_drum_bank][drumnum]++;
                                else if (sample->sfSampleType == RIGHT_SAMPLE)
                                    drum_samples_right[options.opt_drum_bank][drumnum]++;
                                else drum_samples_mono[options.opt_drum_bank][drumnum]++;
                                record_velocity_range(drum, options.opt_drum_bank, drumnum,
                                                      velmin, velmax, sample->sfSampleType);
                            }
                        }
                        else {
                            if (sample->sfSampleType == LEFT_SAMPLE)
                                voice_samples_left[options.opt_bank][wanted_patch]++;
                            else if (sample->sfSampleType == RIGHT_SAMPLE)
                                voice_samples_right[options.opt_bank][wanted_patch]++;
                            else voice_samples_mono[options.opt_bank][wanted_patch]++;
                            record_velocity_range(0, options.opt_bank, wanted_patch,
                                                  velmin, velmax, sample->sfSampleType);
                        }
                    }

                }
            }
        }


    }

    return TRUE;
}







void convert_sf_to_gus(UnSF_Options options){

}



/* creates all the required patch files */
void add_soundfont_patches(UnSF_Options options)
{
    RIFF_CHUNK file, chunk, subchunk;
    FILE *f;
    int i;

    /* SoundFont sample data */
    short *sf_sample_data = NULL;
    int sf_sample_data_size = 0;

    sfPresetHeader *sf_presets = NULL;
    int sf_num_presets = 0;

    sfPresetBag *sf_preset_indexes = NULL;
    int sf_num_preset_indexes = 0;

    sfGenList *sf_preset_generators = NULL;
    int sf_num_preset_generators = 0;

    sfInst *sf_instruments = NULL;
    int sf_num_instruments = 0;

    sfInstBag *sf_instrument_indexes = NULL;
    int sf_num_instrument_indexes = 0;

    /* SoundFont instrument generators */
    sfGenList *sf_instrument_generators = NULL;
    int sf_num_instrument_generators = 0;

    sfSample *sf_samples = NULL;
    int sf_num_samples = 0;



    if (options.opt_verbose)
        printf("\nReading %s\n\n", options.opt_soundfont);
    else
        printf("Reading %s\n", options.opt_soundfont);

    f = fopen(options.opt_soundfont, "rb");
    if (!f) {
        fprintf(stderr, "Error opening file\n");
        err = 1;
        return;
    }

    file.id = get32(f);
    if (file.id != CID_RIFF) {
        fprintf(stderr, "Error: bad SoundFont header\n");
        err = 1;
        goto getout;
    }

    file.size = get32(f);
    calc_end(&file, f);
    file.type = get32(f);
    if (file.type != CID_sfbk) {
        fprintf(stderr, "Error: bad SoundFont header\n");
        err = 1;
        goto getout;
    }

    while (ftell(f) < file.end) {
        chunk.id = get32(f);
        chunk.size = get32(f);
        calc_end(&chunk, f);

        switch (chunk.id) {

            case CID_LIST:
                /* a list of other chunks */
                chunk.type = get32(f);

                while (ftell(f) < chunk.end) {
                    subchunk.id = get32(f);
                    subchunk.size = get32(f);
                    calc_end(&subchunk, f);

                    switch (chunk.type) {

                        case CID_INFO:
                            /* information block */
                            switch (subchunk.id) {

                                case CID_ifil:
                                    if (get16(f) < 2) {
                                        fprintf(stderr, "Error: this is a SoundFont 1.x file, and I only understand version 2 (.sf2)\n");
                                        err = 1;
                                        goto getout;
                                    }
                                    get16(f);
                                    break;

                                case CID_INAM:
                                    print_sf_string(f, "Bank name:", options.opt_no_write);
                                    break;

                                case CID_irom:
                                    print_sf_string(f, "ROM name:", options.opt_no_write);
                                    break;

                                case CID_ICRD:
                                    print_sf_string(f, "Date:", options.opt_no_write);
                                    break;

                                case CID_IENG:
                                    print_sf_string(f, "Made by:", options.opt_no_write);
                                    break;

                                case CID_IPRD:
                                    print_sf_string(f, "Target:", options.opt_no_write);
                                    break;

                                case CID_ICOP:
                                    print_sf_string(f, "Copyright:", options.opt_no_write);
                                    break;

                                case CID_ISFT:
                                    print_sf_string(f, "Tools:", options.opt_no_write);
                                    break;
                            }

                            /* skip unknown chunks and extra data */
                            fseek(f, subchunk.end, SEEK_SET);
                            break;

                        case CID_pdta:
                            /* preset, instrument and sample header data */
                            switch (subchunk.id) {

                                case CID_phdr:
                                    /* preset headers */
                                    sf_num_presets = subchunk.size/38;

                                    if ((sf_num_presets*38 != subchunk.size) ||
                                        (sf_num_presets < 2) || (sf_presets))
                                        BAD_SF();

                                    sf_presets = malloc(sizeof(sfPresetHeader) * sf_num_presets);

                                    for (i=0; i<sf_num_presets; i++) {
                                        fread(sf_presets[i].achPresetName, 20, 1, f);
                                        sf_presets[i].wPreset = get16(f);
                                        sf_presets[i].wBank = get16(f);
                                        sf_presets[i].wPresetBagNdx = get16(f);
                                        sf_presets[i].dwLibrary = get32(f);
                                        sf_presets[i].dwGenre = get32(f);
                                        sf_presets[i].dwMorphology = get32(f);
                                    }
                                    break;

                                case CID_pbag:
                                    /* preset index list */
                                    sf_num_preset_indexes = subchunk.size/4;

                                    if ((sf_num_preset_indexes*4 != subchunk.size) ||
                                        (sf_preset_indexes))
                                        BAD_SF();

                                    sf_preset_indexes = malloc(sizeof(sfPresetBag) * sf_num_preset_indexes);

                                    for (i=0; i<sf_num_preset_indexes; i++) {
                                        sf_preset_indexes[i].wGenNdx = get16(f);
                                        sf_preset_indexes[i].wModNdx = get16(f);
                                    }
                                    break;

                                case CID_pgen:
                                    /* preset generator list */
                                    sf_num_preset_generators = subchunk.size/4;

                                    if ((sf_num_preset_generators*4 != subchunk.size) ||
                                        (sf_preset_generators))
                                        BAD_SF();

                                    sf_preset_generators = malloc(sizeof(sfGenList) * sf_num_preset_generators);

                                    for (i=0; i<sf_num_preset_generators; i++) {
                                        sf_preset_generators[i].sfGenOper = get16(f);
                                        sf_preset_generators[i].genAmount.wAmount = get16(f);
                                    }
                                    break;

                                case CID_inst:
                                    /* instrument names and indices */
                                    sf_num_instruments = subchunk.size/22;

                                    if ((sf_num_instruments*22 != subchunk.size) ||
                                        (sf_num_instruments < 2) || (sf_instruments))
                                        BAD_SF();

                                    sf_instruments = malloc(sizeof(sfInst) * sf_num_instruments);

                                    for (i=0; i<sf_num_instruments; i++) {
                                        fread(sf_instruments[i].achInstName, 20, 1, f);
                                        sf_instruments[i].wInstBagNdx = get16(f);
                                    }
                                    break;

                                case CID_ibag:
                                    /* instrument index list */
                                    sf_num_instrument_indexes = subchunk.size/4;

                                    if ((sf_num_instrument_indexes*4 != subchunk.size) ||
                                        (sf_instrument_indexes))
                                        BAD_SF();

                                    sf_instrument_indexes = malloc(sizeof(sfInstBag) * sf_num_instrument_indexes);

                                    for (i=0; i<sf_num_instrument_indexes; i++) {
                                        sf_instrument_indexes[i].wInstGenNdx = get16(f);
                                        sf_instrument_indexes[i].wInstModNdx = get16(f);
                                    }
                                    break;

                                case CID_igen:
                                    /* instrument generator list */
                                    sf_num_instrument_generators = subchunk.size/4;

                                    if ((sf_num_instrument_generators*4 != subchunk.size) ||
                                        (sf_instrument_generators))
                                        BAD_SF();

                                    sf_instrument_generators = malloc(sizeof(sfGenList) * sf_num_instrument_generators);

                                    for (i=0; i<sf_num_instrument_generators; i++) {
                                        sf_instrument_generators[i].sfGenOper = get16(f);
                                        sf_instrument_generators[i].genAmount.wAmount = get16(f);
                                    }
                                    break;

                                case CID_shdr:
                                    /* sample headers */
                                    sf_num_samples = subchunk.size/46;

                                    if ((sf_num_samples*46 != subchunk.size) ||
                                        (sf_num_samples < 2) || (sf_samples))
                                        BAD_SF();

                                    sf_samples = malloc(sizeof(sfSample) * sf_num_samples);

                                    for (i=0; i<sf_num_samples; i++) {
                                        fread(sf_samples[i].achSampleName, 20, 1, f);
                                        sf_samples[i].dwStart = get32(f);
                                        sf_samples[i].dwEnd = get32(f);
                                        sf_samples[i].dwStartloop = get32(f);
                                        sf_samples[i].dwEndloop = get32(f);
                                        sf_samples[i].dwSampleRate = get32(f);
                                        sf_samples[i].byOriginalKey = get8(f);
                                        sf_samples[i].chCorrection = get8(f);
                                        sf_samples[i].wSampleLink = get16(f);
                                        sf_samples[i].sfSampleType = get16(f);
                                    }
                                    break;
                            }

                            /* skip unknown chunks and extra data */
                            fseek(f, subchunk.end, SEEK_SET);
                            break;

                        case CID_sdta:
                            /* sample data block */
                            switch (subchunk.id) {

                                case CID_smpl:
                                    /* sample waveform (all in one) */
                                    if (sf_sample_data)
                                        BAD_SF();

                                    sf_sample_data_size = subchunk.size / 2;
                                    sf_sample_data = malloc(sizeof(short) * sf_sample_data_size);

                                    for (i=0; i<sf_sample_data_size; i++)
                                        sf_sample_data[i] = get16(f);

                                    break;
                            }

                            /* skip unknown chunks and extra data */
                            fseek(f, subchunk.end, SEEK_SET);
                            break;

                        default:
                            /* unrecognised chunk */
                            fseek(f, chunk.end, SEEK_SET);
                            break;
                    }
                }
                break;

            default:
                /* not a list so we're not interested */
                fseek(f, chunk.end, SEEK_SET);
                break;
        }

        if (feof(f))
            BAD_SF();
    }

    getout:

    if (f)
        fclose(f);

    /* convert SoundFont to .pat format, and add it to the output datafile */
    if (!err) {
        if ((!sf_sample_data) || (!sf_presets) ||
            (!sf_preset_indexes) || (!sf_preset_generators) ||
            (!sf_instruments) || (!sf_instrument_indexes) ||
            (!sf_instrument_generators) || (!sf_samples))
            BAD_SF();

        if (options.opt_verbose)
            printf("\n");

        grab_soundfont_banks(options, sf_num_presets, sf_num_presets, sf_preset_indexes, sf_preset_generators,
                             sf_instruments, sf_instrument_indexes, sf_instrument_generators, sf_samples);
        make_directories();
        sort_velocity_layers();
        shorten_drum_names();
        make_patch_files();
        gen_config_file();
    }


    /* oh, how polite I am... */
    if (sf_sample_data) {
        free(sf_sample_data);
        sf_sample_data = NULL;
    }

    if (sf_presets) {
        free(sf_presets);
        sf_presets = NULL;
    }

    if (sf_preset_indexes) {
        free(sf_preset_indexes);
        sf_preset_indexes = NULL;
    }

    if (sf_preset_generators) {
        free(sf_preset_generators);
        sf_preset_generators = NULL;
    }

    if (sf_instruments) {
        free(sf_instruments);
        sf_instruments = NULL;
    }

    if (sf_instrument_indexes) {
        free(sf_instrument_indexes);
        sf_instrument_indexes = NULL;
    }

    if (sf_instrument_generators) {
        free(sf_instrument_generators);
        sf_instrument_generators = NULL;
    }

    if (sf_samples) {
        free(sf_samples);
        sf_samples = NULL;
    }
}

