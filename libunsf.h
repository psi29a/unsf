#ifndef UNSF_LIBUNSF_H
#define UNSF_LIBUNSF_H

/* set our symbol export visiblity */
#if defined _WIN32 || defined __CYGWIN__
/* ========== NOTE TO WINDOWS DEVELOPERS:
 * If you are compiling for Windows and will link to the static library
 * (libWildMidi.a with MinGW, or wildmidi_static.lib with MSVC, etc),
 * you must define UNSF_STATIC in your project. Otherwise dllimport
 * will be assumed. */
# if defined(UNSF_BUILD) && defined(DLL_EXPORT)		/* building library as a dll for windows */
#  define UNSF_SYMBOL __declspec(dllexport)
# elif defined(UNSF_BUILD) || defined(UNSF_STATIC)	/* building or using static lib for windows */
#  define UNSF_SYMBOL
# else									/* using library dll for windows */
#  define UNSF_SYMBOL __declspec(dllimport)
# endif
#elif defined(UNSF_BUILD)
# if defined(SYM_VISIBILITY)	/* __GNUC__ >= 4, or older gcc with backported feature */
#  define UNSF_SYMBOL __attribute__ ((visibility ("default")))
/*
 # elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
 #  define DD_SYMBOL __attribute__ ((visibility ("default")))
 */
# elif defined(SYM_LDSCOPE)	/* __SUNPRO_C >= 0x550 */
#  define UNSF_SYMBOL __global
# else
#  define UNSF_SYMBOL
# endif
#else
#  define UNSF_SYMBOL
#endif

typedef struct UnSF_Options
{
    int opt_8bit;
    int opt_verbose;
    int opt_veryverbose;
    int opt_bank;
    int opt_drum_bank;
    int opt_header;
    int opt_left_channel;
    int opt_right_channel;
    char *opt_soundfont;
    int opt_small;
    int opt_drum;
    int opt_mono;
    int opt_no_write;
    int opt_adjust_sample_flags;
    int opt_adjust_volume;
    char *basename;
    char *output_directory;
    /* manually set the velocity of either a instrument or drum since most
    applications do not know about the extended patch format. */
    signed char melody_velocity_override[128][128];
    signed char drum_velocity_override[128][128];

    FILE *cfg_fd;
} UnSF_Options;

UNSF_SYMBOL UnSF_Options unsf_initialization();
UNSF_SYMBOL void unsf_convert_sf_to_gus(UnSF_Options *options);


#endif //UNSF_LIBUNSF_H
