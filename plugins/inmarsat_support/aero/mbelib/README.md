[![Build Status](https://travis-ci.org/szechyjs/mbelib.png?branch=master)](https://travis-ci.org/szechyjs/mbelib)

PATENT NOTICE

    This source code is provided for educational purposes only.  It is
    a written description of how certain voice encoding/decoding
    algorythims could be implemented.  Executable objects compiled or 
    derived from this package may be covered by one or more patents.
    Readers are strongly advised to check for any patent restrictions or 
    licencing requirements before compiling or using this source code.

mbelib 1.3.0

        mbelib supports the 7200x4400 bit/s codec used in P25 Phase 1,
        the 7100x4400 bit/s codec used in ProVoice and the "Half Rate"
        3600x2250 bit/s vocoder used in various radio systems.

Example building instructions on Ubuntu:

    sudo apt-get update
    sudo apt-get install git make cmake # Update packages
    git clone <URL of git repository>   # Something like: git@github.com:USERNAME/mbelib.git
    cd mbelib                           # Move into source folder
    mkdir build                         # Create build directory
    cd build                            # Move to build directory
    cmake ..                            # Create Makefile for current system
    make                                # Compiles the library
    sudo make install                   # Library is installed into computer
