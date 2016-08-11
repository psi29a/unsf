/*
 * unsf breaks up sound fonts into GUS type patch files.
 *
 * usage: unsf <name of sound font file>
 *
 * compile: cc -o unsf unsf.c -lm
 *
 * license: cc0
 *
 * To the extent possible under law, the person who associated CC0 with
 * unsf has waived all copyright and related or neighboring rights
 * to unsf.
 *
 * You should have received a copy of the CC0 legalcode along with this
 * work. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__FreeBSD__)
    #include <sys/endian.h>
#elif defined(__APPLE__)
    #include <machine/endian.h>
#else
    #include <endian.h>
#endif

#include "libunsf.h"






/* Bits in modes: */
#define MODES_16BIT	(1<<0)
#define MODES_UNSIGNED	(1<<1)
#define MODES_LOOPING	(1<<2)
#define MODES_PINGPONG	(1<<3)
#define MODES_REVERSE	(1<<4)
#define MODES_SUSTAIN	(1<<5)
#define MODES_ENVELOPE	(1<<6)
#define MODES_FAST_RELEASE	(1<<7)

/* The sampleFlags value, in my experience, is not to be trusted,
 * so the following uses some heuristics to guess whether to
 * set looping and sustain modes. (gl)
 */
static int getmodes(int sampleFlags, int program, int banknum)
{
	int modes;
	int orig_sampleFlags = sampleFlags;

	modes = MODES_ENVELOPE;

      if (opt_8bit)
	 modes |= MODES_UNSIGNED;                      /* signed waveform */
      else
	 modes |= MODES_16BIT;                      /* 16-bit waveform */


	if (sampleFlags == 3) modes |= MODES_FAST_RELEASE;

	/* arbitrary adjustments (look at sustain of vol envelope? ) */

      if (opt_adjust_sample_flags) {

	if (sampleFlags && sf_sustain_mod_env == 0) sampleFlags = 3;
	else if (sampleFlags && sf_sustain_mod_env >= 1000) sampleFlags = 1;
	else if (banknum != 128 && sampleFlags == 1) {
		/* organs, accordians */
		if (program >= 16 && program <= 23) sampleFlags = 3;
		/* strings */
		else if (program >= 40 && program <= 44) sampleFlags = 3;
		/* strings, voice */
		else if (program >= 48 && program <= 54) sampleFlags = 3;
		/* horns, woodwinds */
		else if (program >= 56 && program <= 79) sampleFlags = 3;
		/* lead, pad, fx */
		else if (program >= 80 && program <=103) sampleFlags = 3;
		/* bagpipe, fiddle, shanai */
		else if (program >=109 && program <=111) sampleFlags = 3;
		/* breath noise, ... telephone, helicopter */
		else if (program >=121 && program <=125) sampleFlags = 3;
		/* applause */
		else if (program ==126) sampleFlags = 3;
	}

	if (opt_verbose && orig_sampleFlags != sampleFlags)
		printf("changed sampleFlags from %d to %d\n",
				orig_sampleFlags, sampleFlags);
      }
      else if (sampleFlags == 1) sampleFlags = 3;

	if (sampleFlags == 1 || sampleFlags == 3)
		modes |= MODES_LOOPING;
	if (sampleFlags == 3)
		modes |= MODES_SUSTAIN;
	return modes;
}


/* calculate root pitch */
/* This code is derived from some version of Timidity++ and comes
 * from Takashi Iwai and/or Masanao Izumi (who are not responsible
 * for my interpretation of it). (gl)
 */
static int calc_root_pitch(void)
{
	int root, tune;

	root = sf_key; /* sample originalPitch */
	tune = sf_tune; /* sample pitchCorrection */

	/* tuning */
	/*tune += sf_keyscale; Why did I say this? */
/* ??
		tune += lay->val[SF_coarseTune] * 100
			+ lay->val[SF_fineTune];
*/

	/* it's too high.. */
	if (root >= sf_keymax + 60)
		root -= 60;
/*
	if (lay->set[SF_keyRange] &&
	    root >= HI_VAL(lay->val[SF_keyRange]) + 60)
		root -= 60;

*/

	while (tune <= -100) {
		root++;
		tune += 100;
	}
	while (tune > 0) {
		root--;
		tune -= 100;
	}

    if (root > 0) sp_freq_center = root;
    else sp_freq_center = 60;

    tune = (-tune * 256) / 100;

    if (root > 127)
	return (int)((double)freq_table[127] *
				  bend_coarse[root - 127] * bend_fine[tune]);
    else if (root < 0)
	return (int)((double)freq_table[0] /
				  bend_coarse[-root] * bend_fine[tune]);
    else
	return (int)((double)freq_table[root] * bend_fine[tune]);

}


#define CB_TO_VOLUME(centibel) (255 * (1.0 - ((double)(centibel)/100.0) / (1200.0 * log10(2.0)) ))
/* convert peak volume to linear volume (0-255) */
static unsigned int calc_volume(void)
{
	int v;
	double ret;

	if (!sf_initialAttenuation) return 255;
	v = sf_initialAttenuation;
	if (v < 0) v = 0;
	else if (v > 960) v = 960;
	ret = CB_TO_VOLUME((double)v);
	if (ret < 1.0) return 0;
	if (ret > 255.0) return 255;
	return (unsigned int)ret;
}


#define TO_VOLUME(centibel) (unsigned char)(255 * pow(10.0, -(double)(centibel)/200.0))
/* convert sustain volume to linear volume */
static unsigned char calc_sustain(void)
{
	int level;

	if (!sf_sustain_level) return 250;
	level = TO_VOLUME(sf_sustain_level);
	if (level > 253) level = 253;
	if (level < 100) level = 250; /* Protect against bogus value? This is for PC42c saxes. */
	return (unsigned char)level;
}
static unsigned int calc_mod_sustain(void)
{
	if (!sf_sustain_mod_env) return 250;
	return TO_VOLUME(sf_sustain_mod_env);
}
static void calc_resonance(void)
{
	short val = sf_initialFilterQ;
	//sp_resonance = pow(10.0, (double)val / 2.0 / 200.0) - 1;
	sp_resonance = val;
	if (sp_resonance < 0)
		sp_resonance = 0;
}


/* calculate cutoff/resonance frequency */
static void calc_cutoff(void)
{
	short val;

	if (sf_initialFilterFc < 1) val = 13500;
	else val = sf_initialFilterFc;

	if (sf_modEnvToFilterFc /*&& sf_initialFilterFc*/) {
		sp_modEnvToFilterFc = pow(2.0, ((double)sf_modEnvToFilterFc/1200.0));
	}
	else sp_modEnvToFilterFc = 0;

	if (sf_modLfoToFilterFc /* && sf_initialFilterFc*/) {
		sp_modLfoToFilterFc = pow(2.0, ((double)sf_modLfoToFilterFc/1200.0));
	}
	else sp_modLfoToFilterFc = 0;

	if (sf_mod_env_to_pitch) {
		sp_modEnvToPitch = pow(2.0, ((double)sf_mod_env_to_pitch/1200.0));
	}
	else sp_modEnvToPitch = 0;

	sp_cutoff_freq = TO_HZ(val);
	if (val < 0 || val > 24000) val = 19192;
}




#ifdef LFO_DEBUG
static void
      convert_lfo(char *name, int program, int banknum, int wanted_bank)
#else
static void
convert_lfo (void)
#endif
{
	int   freq = 0, shift = 0;

	if (!sf_modLfoToFilterFc) {
		sp_lfo_depth = sp_lfo_phase_increment = 0;
		return;
	}

	shift = sf_modLfoToFilterFc;
	if (sf_freqModLFO) freq = sf_freqModLFO;

	shift = (int)(pow(2.0, ((double)shift/1200.0)) * VIBRATO_RATE_TUNING);

	sp_lfo_depth = shift;

	if (!freq) freq = 8 * 20;
	else freq = TO_HZ20(freq);

	if (freq < 1) freq = 1;

	sp_lfo_phase_increment = (short)freq;
#ifdef LFO_DEBUG
	fprintf(stderr,"name=%s, bank=%d(%d), prog=%d, freq=%d\n",
			name, banknum, wanted_bank, program, freq);
#endif
}





static int adjust_volume(int start, int length)
{
#if 0
      if (opt_adjust_volume) sp_volume = adjust_volume(sample->dwStart, length);
	 for (i=0; i<length; i++)
	    mem_write16(sf_sample_data[sample->dwStart+i]);
#endif
	    /* Try to determine a volume scaling factor for the sample.
	       This is a very crude adjustment, but things sound more
	       balanced with it. Still, this should be a runtime option. */
	 
	unsigned int  countsamp, numsamps = length;
	unsigned int  higher = 0, highcount = 0;
	short   maxamp = 0, a;
	short  *tmpdta = (short *) sf_sample_data + start;
	double new_vol;
	countsamp = numsamps;
	while (countsamp--) {
		a = *tmpdta++;
		if (a < 0)
		a = -a;
		if (a > maxamp)
		maxamp = a;
	}
	tmpdta = (short *) sf_sample_data + start;
	countsamp = numsamps;
	while (countsamp--) {
		a = *tmpdta++;
		if (a < 0)
		a = -a;
		if (a > 3 * maxamp / 4) {
		    higher += a;
		    highcount++;
		}
	}
	if (highcount)
		higher /= highcount;
	else
		higher = 10000;
	new_vol = (32768.0 * 0.875) / (double) higher;
	return (int)(new_vol * 255.0);
}



















extern char *optarg;
extern int optind, opterr, optopt;



int main(int argc, char *argv[])
{
	int i, c;
	char cfgname[80];
	char *inname;
	char *sep1, *sep2;

	memset(melody_velocity_override, -1, 128*128);
	memset(drum_velocity_override, -1, 128*128);

	while ((c = getopt (argc, argv, "FVvnsdmM:D:")) > 0)
		switch (c) {
			case 'v':
	    			opt_verbose = 1;
	    			break;
			case 'n':
	    			opt_no_write = 1;
	    			break;
			case 's':
	    			opt_small = 1;
	    			break;
			case 'd':
	    			opt_drum = 1;
	    			break;
			case 'm':
	    			opt_mono = 1;
	    			break;
			case 'F':
	    			opt_adjust_sample_flags = 1;
	    			break;
			case 'V':
	    			opt_adjust_volume = 0;
	    			break;
			case 'M':
				sep1 = strchr(optarg, ':');
				sep2 = strchr(optarg, '=');
				if (sep1 && sep2)
				{
				  melody_velocity_override[atoi(optarg)]
				    [atoi(sep1 + 1)] = atoi(sep2 + 1);
				  break;
				} /* if missing, fall through */
			case 'D':
				sep1 = strchr(optarg, ':');
				sep2 = strchr(optarg, '=');
				if (sep1 && sep2)
				{
				  drum_velocity_override[atoi(optarg)]
				    [atoi(sep1 + 1)] = atoi(sep2 + 1);
				  break;
				} /* if missing, fall through */
			default:
				fprintf(stderr, "usage: unsf [-v] [-n] [-s] [-d] [-m] [-F] [-V] [-M <bank>:<instrument>=<layer>]\n"
						"  [-D <bank>:<instrument>=<layer>] <filename>\n");
				return 1;
		}

	if (argc - optind != 1) {
		fprintf(stderr, "usage: unsf [-v] [-n] [-s] [-d] [-m] [-F] [-V] [-M <bank>:<instrument>=<layer>]\n"
		                "  [-D <bank>:<instrument>=<layer>] <filename>\n");
		exit(1);
	}


	inname = strrchr (argv [optind], '/');
	inname = inname ? inname + 1 : argv [optind];

	strcpy(basename, inname);
	inname = strrchr (basename, '.');
	if (inname) inname[0] = '\0';

	for (i = 0; i < strlen(basename); i++) {
		if (basename[i] == ' ') basename[i] = '_';
		else if (basename[i] == '#') basename[i] = '_';
	}

	strcpy(cfgname, basename);
	strcat(cfgname, ".cfg");
	if (!opt_no_write) {
		if ( !(cfg_fd = fopen(cfgname, "wb")) ) return 1;
	}

	opt_soundfont = argv[optind];


    typedef struct UnSF_Optionssss
    {
        int opt_8bit = 0;
        int opt_verbose = 0;
        int opt_veryverbose = 0;
        int opt_bank = 0;
        int opt_drum_bank = 0;
        int opt_header = 0;
        int opt_left_channel = 0;
        int opt_right_channel = 0;
        char *opt_soundfont = NULL;
        int opt_small = 0;
        int opt_drum = 0;
        int opt_mono = 0;
        int opt_no_write = 0;
        int opt_adjust_sample_flags = 0;
        int opt_adjust_volume = 1;
    } UnSF_Optionssss

    UnSF_Options options = { o, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 0 1};

	convert_sf_to_gus();

	if (!opt_no_write) fclose(cfg_fd);

	return 0;
}
