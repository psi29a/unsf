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

    /*
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
    } UnSF_Options; */

    UnSF_Options options = { 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 0, 1};

	convert_sf_to_gus();

	if (!opt_no_write) fclose(cfg_fd);

	return 0;
}
