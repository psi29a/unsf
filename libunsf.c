#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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


/* Function is non ISO standard */
static char *unsf_strdup(const char *s) {
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p) {
        memcpy(p, s, size);
    }
    return p;
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

static char *getname(char *p)
{
    int i, j, e;
    static char buf[21];
    strncpy(buf, p, 20);
    buf[20] = 0;
    for (i = 19; i > 4 && buf[i]==' '; i--) {
        buf[i] = 0;
    }
    e = i + 1;
    if (e < 5) return buf;
    for (i = 0; i < e; i++) {
        if (buf[i] == '/') {
            if (i) buf[i] = '.';
            else buf[i] = ' ';
        }
        else if (buf[i] == '\\') buf[i] = ' ';
        else if (buf[i] == '#') buf[i] = ' ';
        else if (buf[i] == '|') buf[i] = ' ';
        else if (buf[i] == '&') buf[i] = ' ';
        else if (buf[i] == '*') buf[i] = ' ';
        else if (buf[i] == '!') buf[i] = ' ';
        else if (buf[i] == '\'') buf[i] = ' ';
        else if (buf[i] == '"') buf[i] = ' ';
        else if (buf[i] == '?') buf[i] = ' ';
        else if (buf[i] == '~') buf[i] = ' ';
        else if (buf[i] == '[') buf[i] = '-';
        else if (buf[i] == ']') buf[i] = ' ';
        else if (buf[i] == '(') buf[i] = '-';
        else if (buf[i] == ')') buf[i] = ' ';
    }
    for (i = 0; i < e; i++) {
        if (buf[i] == ' ') {
            for (j = i; j < e; j++)
                buf[j] = buf[j+1];
            e--;
        }
    }
    for (i = 0; i < e; i++) {
        if (buf[i] == ' ') {
            for (j = i; j < e; j++)
                buf[j] = buf[j+1];
            e--;
        }
    }
    e = strlen(buf);
    while (e > 3 && buf[e-1] == ' ') {
        buf[e-1] = '\0';
        e--;
    }
    return buf;
}

static void record_velocity_range(UnSF_Options options, int drum, int banknum, int program, int velmin, int velmax,
                                  int type) {
    int i, count;
    VelocityRangeList *vlist;
    char *name;

    if (drum) {
        vlist = drum_velocity[banknum][program];
        name = drum_name[banknum][program];
    }
    else {
        vlist = voice_velocity[banknum][program];
        name = voice_name[banknum][program];
    }

    if (!vlist) {
        vlist = (VelocityRangeList *)malloc(sizeof(VelocityRangeList));
        if (drum) drum_velocity[banknum][program] = vlist;
        else voice_velocity[banknum][program] = vlist;
        vlist->range_count = 0;
    }
    count = vlist->range_count;
    for (i = 0; i < count; i++) {
        if (vlist->velmin[i] == velmin && vlist->velmax[i] == velmax) break;
    }
    if (i >= 128) return;
    vlist->velmin[i] = velmin;
    vlist->velmax[i] = velmax;
    if (i == count) {
        vlist->mono_patches[i] = 0;
        vlist->left_patches[i] = 0;
        vlist->right_patches[i] = 0;
        vlist->other_patches[i] = 0;
        vlist->range_count++;
    }
    if (type == RIGHT_SAMPLE) vlist->right_patches[i]++;
    else if (type == LEFT_SAMPLE) vlist->left_patches[i]++;
    else if (type == MONO_SAMPLE) vlist->mono_patches[i]++;
    else {
        vlist->mono_patches[i]++;
        vlist->other_patches[i]++;
    }

    if (options.opt_veryverbose)
        printf("%s#%d velocity range %d-%d for %s chan %s has %d patches\n", (i==count)? "new ":"",
               vlist->range_count, velmin, velmax,
               name, (type==LEFT_SAMPLE)? "left" : (type==RIGHT_SAMPLE)? "right" : "mono",
               (type==RIGHT_SAMPLE)? vlist->right_patches[i] : (type==LEFT_SAMPLE)? vlist->left_patches[i] :
                                                               vlist->mono_patches[i]);
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
    int pnum, inum, jnum, lnum, drum;
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
                drumset_short_name[options.opt_drum_bank] = unsf_strdup(s);
                sprintf(tmpname, "%s-%s", basename, s);
                drumset_name[options.opt_drum_bank] = unsf_strdup(tmpname);
                if (options.opt_verbose) printf("drumset #%d %s\n", options.opt_drum_bank, s);
            }
        }
        else {
            if (!voice_name[options.opt_bank][wanted_patch]) {
                voice_name[options.opt_bank][wanted_patch] = unsf_strdup(s);
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
                                    drum_name[options.opt_drum_bank][drumnum] = unsf_strdup(s);
                                    if (options.opt_verbose) printf("drumset #%d drum #%d %s\n", options.opt_drum_bank, drumnum, s);
                                }
                                if (sample->sfSampleType == LEFT_SAMPLE)
                                    drum_samples_left[options.opt_drum_bank][drumnum]++;
                                else if (sample->sfSampleType == RIGHT_SAMPLE)
                                    drum_samples_right[options.opt_drum_bank][drumnum]++;
                                else drum_samples_mono[options.opt_drum_bank][drumnum]++;
                                record_velocity_range(options, drum, options.opt_drum_bank, drumnum,
                                                      velmin, velmax, sample->sfSampleType);
                            }
                        }
                        else {
                            if (sample->sfSampleType == LEFT_SAMPLE)
                                voice_samples_left[options.opt_bank][wanted_patch]++;
                            else if (sample->sfSampleType == RIGHT_SAMPLE)
                                voice_samples_right[options.opt_bank][wanted_patch]++;
                            else voice_samples_mono[options.opt_bank][wanted_patch]++;
                            record_velocity_range(options, 0, options.opt_bank, wanted_patch,
                                                  velmin, velmax, sample->sfSampleType);
                        }
                    }

                }
            }
        }


    }

    return TRUE;
}

static void make_directories(UnSF_Options options)
{
    int i, rcode, tonebank_count = 0;
    char tmpname[80];

    printf("Making bank directories.\n");

    for (i = 0; i < 128; i++) if (tonebank[i]) tonebank_count++;

    for (i = 0; i < 128; i++) {
        if (tonebank[i]) {
            if (tonebank_count > 1) {
                sprintf(tmpname, "%s-B%d", basename, i);
                tonebank_name[i] = unsf_strdup(tmpname);
            }
            else tonebank_name[i] = unsf_strdup(basename);
            if (options.opt_no_write) continue;
            if ( (rcode = access(tonebank_name[i], R_OK|W_OK|X_OK)) )
                rcode=mkdir(tonebank_name[i], S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            if (rcode) {
                fprintf(stderr, "Could not create directory %s\n", tonebank_name[i]);
                exit(1);
            }
        }
    }
    if (options.opt_no_write) return;
    for (i = 0; i < 128; i++) {
        if (drumset_name[i]) {
            if ( (rcode = access(drumset_name[i], R_OK|W_OK|X_OK)) )
                rcode=mkdir(drumset_name[i], S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            if (rcode) {
                fprintf(stderr, "Could not create directory %s\n", drumset_name[i]);
                exit(1);
            }
        }
    }
}


static void sort_velocity_layers(void)
{
    int i, j, k, velmin, velmax, velcount, left_patches, right_patches, mono_patches;
    int width, widest;
    VelocityRangeList *vlist;


    for (i = 0; i < 128; i++) {
        if (tonebank[i]) {
            for (j = 0; j < 128; j++) {
                if (voice_name[i][j]) {
                    vlist = voice_velocity[i][j];
                    if (vlist) {
                        velcount = vlist->range_count;
                        widest = 0;
                        width = vlist->velmax[0] - vlist->velmin[0];
                        for (k = 1; k < velcount; k++) {
                            if (vlist->velmax[k] - vlist->velmin[k] > width) {
                                widest = k;
                                width = vlist->velmax[k] - vlist->velmin[k];
                            }
                        }
                        if (melody_velocity_override[i][j] != -1)
                            widest = melody_velocity_override[i][j];
                        if (widest) {
                            velmin = vlist->velmin[0];
                            velmax = vlist->velmax[0];
                            mono_patches = vlist->mono_patches[0];
                            left_patches = vlist->left_patches[0];
                            right_patches = vlist->right_patches[0];

                            vlist->velmin[0] = vlist->velmin[widest];
                            vlist->velmax[0] = vlist->velmax[widest];
                            vlist->mono_patches[0] = vlist->mono_patches[widest];
                            vlist->left_patches[0] = vlist->left_patches[widest];
                            vlist->right_patches[0] = vlist->right_patches[widest];

                            vlist->velmin[widest] = velmin;
                            vlist->velmax[widest] = velmax;
                            vlist->mono_patches[widest] = mono_patches;
                            vlist->left_patches[widest] = left_patches;
                            vlist->right_patches[widest] = right_patches;
                        }
                    }
                    else continue;
                }
            }
        }
    }
    for (i = 0; i < 128; i++) {
        if (drumset_name[i]) {
            for (j = 0; j < 128; j++) {
                if (drum_name[i][j]) {
                    vlist = drum_velocity[i][j];
                    if (vlist) {
                        velcount = vlist->range_count;
                        widest = 0;
                        width = vlist->velmax[0] - vlist->velmin[0];
                        for (k = 1; k < velcount; k++) {
                            if (vlist->velmax[k] - vlist->velmin[k] > width) {
                                widest = k;
                                width = vlist->velmax[k] - vlist->velmin[k];
                            }
                        }
                        if (drum_velocity_override[i][j] != -1)
                            widest = drum_velocity_override[i][j];
                        if (widest) {
                            velmin = vlist->velmin[0];
                            velmax = vlist->velmax[0];
                            mono_patches = vlist->mono_patches[0];
                            left_patches = vlist->left_patches[0];
                            right_patches = vlist->right_patches[0];

                            vlist->velmin[0] = vlist->velmin[widest];
                            vlist->velmax[0] = vlist->velmax[widest];
                            vlist->mono_patches[0] = vlist->mono_patches[widest];
                            vlist->left_patches[0] = vlist->left_patches[widest];
                            vlist->right_patches[0] = vlist->right_patches[widest];

                            vlist->velmin[widest] = velmin;
                            vlist->velmax[widest] = velmax;
                            vlist->mono_patches[widest] = mono_patches;
                            vlist->left_patches[widest] = left_patches;
                            vlist->right_patches[widest] = right_patches;
                        }
                    }
                    else continue;
                }
            }
        }
    }
}

static void shorten_drum_names()
{
    int i, j, velcount, right_patches;
    VelocityRangeList *vlist;

    for (i = 0; i < 128; i++) {
        if (drumset_name[i]) {
            for (j = 0; j < 128; j++) {
                if (drum_name[i][j]) {
                    vlist = drum_velocity[i][j];
                    if (vlist) {
                        velcount = vlist->range_count;  /* TODO: set but not used */
                        right_patches = vlist->right_patches[0];
                    }
                    else {
                        continue;
                    }
                    if (right_patches) {
                        char *dnm = drum_name[i][j];
                        int name_len = strlen(dnm);
                        if (name_len > 4 && dnm[name_len-1] == 'L' &&
                            dnm[name_len-2] == '-')
                            dnm[name_len-2] = '\0';
                    }
                }
            }
        }
    }
}

/* converts loaded SoundFont data */
static int grab_soundfont(UnSF_Options options, int num, int drum, char *name, int wanted_velmin, int wanted_velmax,
                          int sf_num_presets, sfPresetHeader *sf_presets,
                          sfPresetBag * sf_preset_indexes, sfGenList *sf_preset_generators,
                          sfInst *sf_instruments, sfInstBag *sf_instrument_indexes,
                          sfGenList * sf_instrument_generators, sfSample * sf_samples)
{
    sfPresetHeader *pheader;
    sfPresetBag *pindex;
    sfGenList *pgen;
    sfInst *iheader;
    sfInstBag *iindex;
    sfGenList *igen;
    sfGenList *global_izone;
    sfGenList *global_pzone;
    sfSample *sample;
    int pindex_count;
    int pgen_count;
    int iindex_count;
    int igen_count;
    int pnum, inum, jnum, lnum;
    int global_izone_index;
    int global_izone_count;
    int global_pzone_index;
    int global_pzone_count;
    int wanted_patch, wanted_bank;
    int keymin, keymax;
    int wanted_keymin, wanted_keymax;
    int velmin, velmax;
    int waiting_room_full;
    int i;
    char *s;

    EMPTY_WHITE_ROOM waiting_list[MAX_WAITING];
    int waiting_list_count;

    if (drum) {
        if (options.opt_drum) {
            wanted_patch = options.opt_drum_bank;
            wanted_bank = 0;
        }
        else {
            wanted_patch = options.opt_drum_bank;
            wanted_bank = 128;
        }
        wanted_keymin = num;
        wanted_keymax = num;
    }
    else {
        wanted_patch = num;
        wanted_bank = options.opt_bank;
        wanted_keymin = 0;
        wanted_keymax = 127;
    }

    /* search for the desired preset */
    for (pnum=0; pnum<sf_num_presets; pnum++) {
        int global_preset_layer, global_preset_velmin, global_preset_velmax, preset_velmin, preset_velmax;
        int global_preset_keymin, global_preset_keymax, preset_keymin, preset_keymax;

        pheader = &sf_presets[pnum];

        if ((pheader->wPreset == wanted_patch) && (pheader->wBank == wanted_bank)) {
            /* find what substructures it uses */
            pindex = &sf_preset_indexes[pheader->wPresetBagNdx];
            pindex_count = pheader[1].wPresetBagNdx - pheader[0].wPresetBagNdx;

            if (pindex_count < 1)
                return FALSE;

            /* prettify the preset name */
            s = pheader->achPresetName;

            i = strlen(s)-1;
            while ((i >= 0) && (isspace(s[i]))) {
                s[i] = 0;
                i--;
            }

            if (options.opt_verbose) printf("Grabbing %s%s -> %s\n", options.opt_right_channel? "R ":"L ", s, name);
            else if (!options.opt_no_write) { printf("."); fflush(stdout); }

            waiting_list_count = 0;
            waiting_room_full = FALSE;

            global_pzone = NULL;
            global_pzone_index = -1;
            global_pzone_count = 0;

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

                if (pgen_count < 0) break;
                //velmin = 0;
                //velmax = 127;

                if (global_preset_velmin >= 0) preset_velmin = global_preset_velmin;
                if (global_preset_velmax >= 0) preset_velmax = global_preset_velmax;
                if (global_preset_keymin >= 0) preset_keymin = global_preset_keymin;
                if (global_preset_keymax >= 0) preset_keymax = global_preset_keymax;

                if (pgen_count > 0 && pgen[pgen_count-1].sfGenOper != SFGEN_instrument) { /* global preset zone */
                    global_pzone = pgen;
                    global_pzone_index = inum;
                    global_pzone_count = pgen_count;
                    global_preset_layer = TRUE;
                }
                else global_preset_layer = FALSE;

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

#if 0
                    if (pgen[0].sfGenOper == SFGEN_keyRange)
		  if ((pgen[0].genAmount.ranges.byHi < keymin) ||
		      (pgen[0].genAmount.ranges.byLo > keymax))
		     continue;
#endif

                    iheader = &sf_instruments[pgen[pgen_count-1].genAmount.wAmount];

                    iindex = &sf_instrument_indexes[iheader->wInstBagNdx];
                    iindex_count = iheader[1].wInstBagNdx - iheader[0].wInstBagNdx;

                    global_instrument_velmin = instrument_velmin = -1;
                    global_instrument_velmax = instrument_velmax = -1;
                    global_instrument_keymin = instrument_keymin = -1;
                    global_instrument_keymax = instrument_keymax = -1;


                    global_izone = NULL;
                    global_izone_index = -1;
                    global_izone_count = 0;

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
                                instrument_velmin = igen[jnum].genAmount.ranges.byLo;
                                instrument_velmax = igen[jnum].genAmount.ranges.byHi;
                                if (global_instrument_layer) {
                                    global_instrument_velmin = instrument_velmin;
                                    global_instrument_velmax = instrument_velmax;
                                }
                            }
                        }

                        if (igen_count > 0 && igen[0].sfGenOper == SFGEN_keyRange) {
                            instrument_keymin = igen[0].genAmount.ranges.byLo;
                            instrument_keymax = igen[0].genAmount.ranges.byHi;
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
#if 0
                        if (!drum && !wanted_bank && wanted_patch == 16) {
	fprintf(stderr,"velmin=%d velmax=%d wanted_velmin=%d wanted_velmax=%d\n",
			velmin, velmax, wanted_velmin, wanted_velmax);
	fprintf(stderr,"instrument_velmin=%d instrument_velmax=%d preset_velmin=%d preset_velmax=%d\n",
			instrument_velmin, instrument_velmax, preset_velmin, preset_velmax);
	fprintf(stderr,"keymin=%d keymax=%d wanted_keymin=%d wanted_keymax=%d\n",
			keymin, keymax, wanted_keymin, wanted_keymax);
	fprintf(stderr,"instrument_keymin=%d instrument_keymax=%d preset_keymin=%d preset_keymax=%d\n",
			instrument_keymin, instrument_keymax, preset_keymin, preset_keymax);
}
#endif
                        if (velmin != wanted_velmin || velmax != wanted_velmax) continue;
                        if (drum && (wanted_keymin < keymin || wanted_keymin > keymax)) continue;
                        if (!drum && (keymin < wanted_keymin || keymax > wanted_keymax)) continue;

                        /* find what sample we should use */
                        if ((igen_count > 0) &&
                            (igen[igen_count-1].sfGenOper == SFGEN_sampleID)) {

                            sample = &sf_samples[igen[igen_count-1].genAmount.wAmount];

                            /* sample->wSampleLink is the link?? */
                            /* lsample = &sf_samples[sample->wSampleLink] */


                            if (sample->sfSampleType & LINKED_SAMPLE) continue; /* linked */

                            s = sample->achSampleName;

                            i = strlen(s)-1;

                            if (s[i] == 'L') sample->sfSampleType = LEFT_SAMPLE;
                            if (s[i] == 'R') sample->sfSampleType = RIGHT_SAMPLE;

                            if (sample->sfSampleType == LEFT_SAMPLE && !options.opt_left_channel) continue;
                            if (sample->sfSampleType == RIGHT_SAMPLE && !options.opt_right_channel) continue;
                            if (sample->sfSampleType == MONO_SAMPLE && options.opt_right_channel) continue;

                            /* prettify the sample name */

                            if (options.opt_verbose) {
                                int j = i - 3;
                                if (j < 0) j = 0;
                                while (j <= i) {
                                    if (s[j] == 'R') break;
                                    if (s[j] == 'L' && j < i && s[j+1] == 'o') { j++; continue; }
                                    if (s[j] == 'L') break;
                                    j++;
                                }
                                if (j <= i) {
                                    if (s[j] == 'R' && sample->sfSampleType != RIGHT_SAMPLE)
                                        printf("Note that sample name %s is not a right sample\n", s);
                                    //if (s[j] == 'R' && sample->sfSampleType == RIGHT_SAMPLE)
                                    //	     printf("OK -- sample name %s is a right sample\n", s);
                                    if (s[j] == 'L' && sample->sfSampleType != LEFT_SAMPLE)
                                        printf("Note that sample name %s is not a left sample\n", s);
                                    //if (s[j] == 'L' && sample->sfSampleType == LEFT_SAMPLE)
                                    //	     printf("OK -- sample name %s is a left sample\n", s);
                                }
                            }

                            while ((i >= 0) && (isspace(s[i]))) {
                                s[i] = 0;
                                i--;
                            }

                            if (sample->sfSampleType & 0x8000) {
                                printf("\nThis SoundFont uses AWE32 ROM data in sample %s\n", s);
                                if (options.opt_veryverbose)
                                    printf("\n");
                                return FALSE;
                            }

                            /* add this sample to the waiting list */
                            if (waiting_list_count < MAX_WAITING) {
                                if (options.opt_veryverbose)
                                    printf("  - sample %s\n", s);

                                waiting_list[waiting_list_count].sample = sample;
                                waiting_list[waiting_list_count].igen = igen;
                                waiting_list[waiting_list_count].pgen = pgen;
                                waiting_list[waiting_list_count].global_izone = global_izone;
                                waiting_list[waiting_list_count].global_pzone = global_pzone;
                                waiting_list[waiting_list_count].igen_count = igen_count;
                                waiting_list[waiting_list_count].pgen_count = pgen_count;
                                waiting_list[waiting_list_count].global_izone_count = global_izone_count;
                                waiting_list[waiting_list_count].global_pzone_count = global_pzone_count;
                                waiting_list[waiting_list_count].volume = 1.0;
                                waiting_list[waiting_list_count].stereo_mode = sample->sfSampleType;
                                waiting_list_count++;

                            }
                            else
                                waiting_room_full = TRUE;
                        }
                        else if (igen_count > 0) { /* global instrument zone */

                            global_izone = igen;
                            global_izone_index = lnum;
                            global_izone_count = igen_count;
                        }
                    }
                }
            }

            if (waiting_room_full)
                printf("Warning: too many layers in this instrument!\n");

            if (waiting_list_count > 0) {
                int pcount, vcount, k;
                VelocityRangeList *vlist;
                if (drum) vlist = drum_velocity[wanted_patch][wanted_keymin];
                else vlist = voice_velocity[wanted_bank][wanted_patch];
                if (!vlist) {
                    fprintf(stderr,"\nNo record found for %s, keymin=%d patch=%d bank=%d\n",
                            name, wanted_keymin, wanted_patch, wanted_bank);
                    return FALSE;
                }
                vcount = vlist->range_count;
                for (k = 0; k < vcount; k++)
                    if (vlist->velmin[k] == wanted_velmin && vlist->velmax[k] == wanted_velmax) break;
                if (k == vcount) {
                    fprintf(stderr,"\n%s patches were requested for an unknown velocity range.\n", name);
                    return FALSE;
                }
                if (options.opt_right_channel) pcount = vlist->right_patches[k];
                else pcount = vlist->left_patches[k] + vlist->mono_patches[k];
#if 0
                if (options.opt_header && pcount > waiting_list_count && options.opt_left_channel) {
		    while (pcount > waiting_list_count && vlist->right_patches[k] > 1 && vlist->left_patches[k] > 1) {
			    vlist->right_patches[k]--;
			    vlist->left_patches[k]--;
			    pcount--;
		    }
		    while (pcount > waiting_list_count && vlist->mono_patches[k] > 1) {
			    vlist->mono_patches[k]--;
			    pcount--;
		    }
	    }
#endif
                if (pcount != waiting_list_count) {
                    fprintf(stderr,"\nFor %sinstrument %s %s found %d samples when there should be %d samples.\n",
                            options.opt_header? "header of ": "", name,
                            options.opt_left_channel? "left/mono": "right",
                            waiting_list_count, pcount);
                    fprintf(stderr,"\tkeymin=%d keymax=%d patch=%d bank=%d, velmin=%d, velmax=%d\n",
                            wanted_keymin, wanted_keymax, wanted_patch, wanted_bank,
                            wanted_velmin, wanted_velmax);
                    fprintf(stderr,"\tleft patches %d, right patches %d, mono patches %d\n",
                            vlist->left_patches[k],
                            vlist->right_patches[k],
                            vlist->mono_patches[k]);
                    return FALSE;
                }
                if (options.opt_verbose && vlist->other_patches[k]) {
                    fprintf(stderr,"\nFor instrument %s found %d samples in unknown channel.\n",
                            name, vlist->other_patches[k]);
                }
                if (drum) return grab_soundfont_sample(name, wanted_keymin, wanted_patch, wanted_bank);
                else return grab_soundfont_sample(name, wanted_patch, wanted_bank, wanted_bank);
            }
            else {
                fprintf(stderr,"\nStrange... no valid layers found in instrument %s bank %d prog %d\n",
                        name, drum? wanted_patch:wanted_bank, drum? wanted_keymin:wanted_patch);
                return FALSE;
            }
            if (options.opt_veryverbose)
                printf("\n");

            return (waiting_list_count > 0);
        }
    }

    return FALSE;
}

static void make_patch_files(UnSF_Options options, int sf_num_presets, sfPresetHeader *sf_presets,
                             sfPresetBag * sf_preset_indexes, sfGenList *sf_preset_generators,
                             sfInst *sf_instruments, sfInstBag *sf_instrument_indexes,
                             sfGenList * sf_instrument_generators, sfSample * sf_samples)
{
    int i, j, k, velcount, right_patches;
    char tmpname[80];
    FILE *pf;
    VelocityRangeList *vlist;
    int abort_this_one;
    int wanted_velmin, wanted_velmax;

    /* scratch buffer for generating new patch files */
    unsigned char *mem = NULL;
    int mem_size = 0;
    int mem_alloced = 0;


    printf("Melodic patch files.\n");
    for (i = 0; i < 128; i++) {
        abort_this_one = FALSE;
        if (tonebank[i]) for (j = 0; j < 128; j++) if (voice_name[i][j]) {
                    abort_this_one = FALSE;
                    vlist = voice_velocity[i][j];
                    if (vlist) velcount = vlist->range_count;
                    else velcount = 1;
                    if (options.opt_small) velcount = 1;
                    options.opt_bank = i;
                    options.opt_header = TRUE;
                    for (k = 0; k < velcount; k++) {
                        if (vlist) {
                            wanted_velmin = vlist->velmin[k];
                            wanted_velmax = vlist->velmax[k];
                            right_patches = vlist->right_patches[k];
                        }
                        else {
                            wanted_velmin = 0;
                            wanted_velmax = 127;
                            right_patches = voice_samples_right[i][j];
                        }
                        options.opt_left_channel = TRUE;
                        options.opt_right_channel = FALSE;
                        if (!grab_soundfont(options, j, FALSE, voice_name[i][j], wanted_velmin, wanted_velmax,
                                            sf_num_presets, sf_presets, sf_preset_indexes, sf_preset_generators,
                                            sf_instruments, sf_instrument_indexes, sf_instrument_generators,
                                            sf_samples)) {
                            fprintf(stderr, "Could not create patch %s for bank %s\n",
                                    voice_name[i][j], tonebank_name[i]);
                            fprintf(stderr,"\tlayer %d of %d layer(s)\n", k+1, velcount);
                            if (voice_velocity[i][j]) free(voice_velocity[i][j]);
                            voice_velocity[i][j] = NULL;
                            abort_this_one = TRUE;
                            break;
                        }
                        options.opt_header = FALSE;
                        if (abort_this_one) continue;
                        if (vlist) right_patches = vlist->right_patches[k];
                        if (right_patches && !options.opt_mono) {
                            options.opt_left_channel = FALSE;
                            options.opt_right_channel = TRUE;
                            if (!grab_soundfont(options, j, FALSE, voice_name[i][j], wanted_velmin, wanted_velmax,
                                                sf_num_presets, sf_presets, sf_preset_indexes, sf_preset_generators,
                                                sf_instruments, sf_instrument_indexes, sf_instrument_generators,
                                                sf_samples)) {
                                fprintf(stderr, "Could not create right patch %s for bank %s\n",
                                        voice_name[i][j], tonebank_name[i]);
                                fprintf(stderr,"\tlayer %d of %d layer(s)\n", k+1, velcount);
                                if (voice_velocity[i][j]) free(voice_velocity[i][j]);
                                voice_velocity[i][j] = NULL;
                                abort_this_one = TRUE;
                                break;
                            }
                        }
                    }
                    if (abort_this_one || options.opt_no_write) continue;
                    sprintf(tmpname, "%s/%s.pat", tonebank_name[i], voice_name[i][j]);
                    if (!(pf = fopen(tmpname, "wb"))) {
                        fprintf(stderr, "\nCould not open patch file %s\n", tmpname);
                        if (voice_velocity[i][j]) free(voice_velocity[i][j]);
                        voice_velocity[i][j] = NULL;
                        continue;
                    }
                    if ( fwrite(mem, 1, mem_size, pf) != mem_size ) {
                        fprintf(stderr, "\nCould not write to patch file %s\n", tmpname);
                        if (voice_velocity[i][j]) free(voice_velocity[i][j]);
                        voice_velocity[i][j] = NULL;
                    }
                    fclose(pf);
                }
    }
    printf("\nDrum patch files.\n");
    for (i = 0; i < 128; i++) {
        abort_this_one = FALSE;
        if (drumset_name[i]) for (j = 0; j < 128; j++) if (drum_name[i][j]) {
                    abort_this_one = FALSE;
                    vlist = drum_velocity[i][j];
                    if (vlist) velcount = vlist->range_count;
                    else velcount = 1;
                    if (!vlist) fprintf(stderr, "Uh oh, drum #%d %s has no velocity list\n", i, drumset_name[i]);
                    if (options.opt_small) velcount = 1;
                    options.opt_drum_bank = i;
                    options.opt_header = TRUE;
                    for (k = 0; k < velcount; k++) {
                        if (vlist) {
                            wanted_velmin = vlist->velmin[k];
                            wanted_velmax = vlist->velmax[k];
                            right_patches = vlist->right_patches[k];
                        }
                        else {
                            wanted_velmin = 0;
                            wanted_velmax = 127;
                            right_patches = drum_samples_right[i][j];
                        }
                        options.opt_left_channel = TRUE;
                        options.opt_right_channel = FALSE;
                        if (!grab_soundfont(options, j, TRUE, drum_name[i][j], wanted_velmin, wanted_velmax,
                                            sf_num_presets, sf_presets, sf_preset_indexes, sf_preset_generators,
                                            sf_instruments, sf_instrument_indexes, sf_instrument_generators, sf_samples)) {
                            fprintf(stderr, "Could not create left/mono patch %s for bank %s\n",
                                    drum_name[i][j], drumset_name[i]);
                            fprintf(stderr,"\tlayer %d of %d layer(s)\n", k+1, velcount);
                            if (drum_velocity[i][j]) free(drum_velocity[i][j]);
                            drum_velocity[i][j] = NULL;
                            abort_this_one = TRUE;
                            break;
                        }
                        options.opt_header = FALSE;
                        if (abort_this_one) continue;
                        if (vlist) right_patches = vlist->right_patches[k];
                        if (right_patches && !options.opt_mono) {
                            options.opt_left_channel = FALSE;
                            options.opt_right_channel = TRUE;
                            if (!grab_soundfont(options, j, TRUE, drum_name[i][j], wanted_velmin, wanted_velmax,
                                        sf_num_presets, sf_presets, sf_preset_indexes, sf_preset_generators,
                                        sf_instruments, sf_instrument_indexes, sf_instrument_generators, sf_samples)) {
                                fprintf(stderr, "Could not create right patch %s for bank %s\n",
                                        drum_name[i][j], drumset_name[i]);
                                fprintf(stderr,"\tlayer %d of %d layer(s)\n", k+1, velcount);
                                if (drum_velocity[i][j]) free(drum_velocity[i][j]);
                                drum_velocity[i][j] = NULL;
                                abort_this_one = TRUE;
                                break;
                            }
                        }
                    }
                    if (abort_this_one || options.opt_no_write) continue;
                    sprintf(tmpname, "%s/%s.pat", drumset_name[i], drum_name[i][j]);
                    if (!(pf = fopen(tmpname, "wb"))) {
                        fprintf(stderr, "\nCould not open patch file %s\n", tmpname);
                        if (drum_velocity[i][j]) free(drum_velocity[i][j]);
                        drum_velocity[i][j] = NULL;
                        continue;
                    }
                    if ( fwrite(mem, 1, mem_size, pf) != mem_size ) {
                        fprintf(stderr, "\nCould not write to patch file %s\n", tmpname);
                        if (drum_velocity[i][j]) free(drum_velocity[i][j]);
                        drum_velocity[i][j] = NULL;
                    }
                    fclose(pf);
                }
    }
    printf("\n");
}

static void gen_config_file(UnSF_Options options)
{
    int i, j, velcount, right_patches;
    VelocityRangeList *vlist;

    if (options.opt_no_write) return;

    printf("Generating config file.\n");

    for (i = 0; i < 128; i++) {
        if (tonebank[i]) {
            fprintf(cfg_fd, "\nbank %d #N %s\n", i, tonebank_name[i]);
            for (j = 0; j < 128; j++) {
                if (voice_name[i][j]) {
                    vlist = voice_velocity[i][j];
                    if (vlist) {
                        velcount = vlist->range_count;
                        right_patches = vlist->right_patches[0];
                    }
                    else {
                        fprintf(cfg_fd, "\t# %d %s could not be extracted\n", j,
                                voice_name[i][j]);
                        continue;
                    }
                    fprintf(cfg_fd, "\t%d %s/%s", j,
                            tonebank_name[i], voice_name[i][j]);
                    if (velcount > 1) fprintf(cfg_fd, "\t# %d velocity ranges", velcount);
                    if (right_patches) {
                        if (velcount == 1) fprintf(cfg_fd, "\t# stereo");
                        else fprintf(cfg_fd,", stereo");
                    }
                    fprintf(cfg_fd, "\n");
                }
            }
        }
    }
    for (i = 0; i < 128; i++) {
        if (drumset_name[i]) {
            fprintf(cfg_fd, "\ndrumset %d #N %s\n", i, drumset_short_name[i]);
            for (j = 0; j < 128; j++) {
                if (drum_name[i][j]) {
                    vlist = drum_velocity[i][j];
                    if (vlist) {
                        velcount = vlist->range_count;
                        right_patches = vlist->right_patches[0];
                    }
                    else {
                        fprintf(cfg_fd, "\t# %d %s could not be extracted\n", j,
                                drum_name[i][j]);
                        continue;
                    }
                    fprintf(cfg_fd, "\t%d %s/%s", j,
                            drumset_name[i], drum_name[i][j]);
                    if (velcount > 1) fprintf(cfg_fd, "\t# %d velocity ranges", velcount);
                    if (right_patches) {
                        if (velcount == 1) fprintf(cfg_fd, "\t# stereo");
                        else fprintf(cfg_fd,", stereo");
                    }
                    fprintf(cfg_fd, "\n");
                }
            }
        }
    }
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

        grab_soundfont_banks(options, sf_num_presets, sf_presets, sf_preset_indexes, sf_preset_generators,
                             sf_instruments, sf_instrument_indexes, sf_instrument_generators, sf_samples);
        make_directories(options);
        sort_velocity_layers();
        shorten_drum_names();
        make_patch_files(options, sf_num_presets, sf_presets, sf_preset_indexes, sf_preset_generators,
                         sf_instruments, sf_instrument_indexes, sf_instrument_generators, sf_samples);
        gen_config_file(options);
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

