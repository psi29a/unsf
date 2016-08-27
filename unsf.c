/*
 * unsf breaks up sound fonts into GUS type patch files.
 *
 * usage: unsf <name of sound font file>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "libunsf.h"


extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char *argv[]) {
    int i, c;
    char cfgname[80];
    char *inname;
    char *sep1, *sep2;

    UnSF_Options options = unsf_initialization();

    while ((c = getopt(argc, argv, "FVvnsdmM:D:")) > 0)
        switch (c) {
            case 'v':
                if (options.opt_verbose) options.opt_veryverbose = 1;
                else options.opt_verbose = 1;
                break;
            case 'n':
                options.opt_no_write = 1;
                break;
            case 's':
                options.opt_small = 1;
                break;
            case 'd':
                options.opt_drum = 1;
                break;
            case 'm':
                options.opt_mono = 1;
                break;
            case 'F':
                options.opt_adjust_sample_flags = 1;
                break;
            case 'V':
                options.opt_adjust_volume = 0;
                break;
            case 'M':
                sep1 = strchr(optarg, ':');
                sep2 = strchr(optarg, '=');
                if (sep1 && sep2) {
                    options.melody_velocity_override[atoi(optarg)][atoi(sep1 + 1)] = atoi(sep2 + 1);
                    break;
                } /* if missing, fall through */
            case 'D':
                sep1 = strchr(optarg, ':');
                sep2 = strchr(optarg, '=');
                if (sep1 && sep2) {
                    options.drum_velocity_override[atoi(optarg)][atoi(sep1 + 1)] = atoi(sep2 + 1);
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


    inname = strrchr(argv[optind], '/');
    inname = inname ? inname + 1 : argv[optind];

    if (!(options.basename = malloc(sizeof(char) * strlen(inname) + 1))) {
        fprintf(stderr, "Memory allocation of %lu failed\n", strlen(inname) + 1);
        exit(1);
    }


    strcpy(options.basename, inname);
    inname = strrchr(options.basename, '.');
    if (inname) inname[0] = '\0';

    for (i = 0; i < strlen(options.basename); i++) {
        if (options.basename[i] == ' ') options.basename[i] = '_';
        else if (options.basename[i] == '#') options.basename[i] = '_';
    }

    strcpy(cfgname, options.basename);
    strcat(cfgname, ".cfg");
    if (!options.opt_no_write) {
        if (!(options.cfg_fd = fopen(cfgname, "wb"))) {
            free(options.basename);
            return 1;
        }
    }

    options.opt_soundfont = argv[optind];

    printf("Reading %s\n", options.opt_soundfont);
    unsf_convert_sf_to_gus(&options);

    free(options.basename);
    printf("Writing out %s\n", cfgname);
    if (!options.opt_no_write) fclose(options.cfg_fd);
    printf("Finished!\n");

    return 0;
}
