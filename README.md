UnSF
====

[![CC0 License](https://img.shields.io/badge/license-cc0-blue.svg)](https://creativecommons.org/publicdomain/zero/1.0/) [![Build Status](https://img.shields.io/travis/psi29a/unsf.svg)](https://travis-ci.org/psi29a/unsf) [![Coverity Status](https://img.shields.io/coverity/scan/9902.svg)](https://scan.coverity.com/projects/psi29a-unsf)


UnSF is a tool to convert SoundFont (sf2) files into GUS patches.  

Some reasons you might want to do such a thing are: 
 * Your midi player can use GUS patches but doesn't know how to read soundfonts
 * You want a convenient way of substituting patches or reassigning banks of instruments by editing GUS config files
 * You need to modify patches by changing amplitude, stripping envelopes, or make other changes that can easily be done by attaching options in config files.

License: CC0 - 1.0

History
=======

unsf.c is derived from the Allegro tool pat2dat.c, and uses, presumably, the part of that program which was written originally by George Foot. Also, some code is adapted from routines in sndfont.c. All of which was original under the Allegro "beerware" license.
