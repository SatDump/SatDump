# Libraries

**Libraries included in libsatdump_core :**
- [spdlog](https://github.com/gabime/spdlog), for logging
- [fmt](https://github.com/fmtlib/fmt), as part of spdlog
- [ImGui](https://github.com/ocornut/imgui), for the user interface
- [ctpl](https://github.com/vit-vit/ctpl), for thread pools used over the program
- [libcorrect](https://github.com/quiet/libcorrect), for Reed-Solomon decoding
- [libaec](https://gitlab.dkrz.de/k202009/libaec), with OpenSatelliteProject's path
- [deepspace-turbo](https://github.com/geeanlooca/deepspace-turbo), used for Turbo decoding
- [dlfcn-win32](https://github.com/dlfcn-win32/dlfcn-win32), to use the usual dlopen() functions
- [libjpeg](https://ijg.org/), from the Independent JPEG Group
- [miniz](https://github.com/richgel999/miniz), used to decompress ZIP files in some decoders
- [MuParser](https://github.com/beltoforion/muparser), for expression parsing (such as in composites)
- [OpenCL C++ Headers](https://github.com/KhronosGroup/OpenCL-CLHPP)
- [libpredict](https://github.com/la1k/libpredict), used for orbit prediction
- [RapidXML](http://rapidxml.sourceforge.net/), for XML Parsing
- [Nlhohmann's JSON](https://github.com/nlohmann/json), for JSON and CBOR encoding/parsing
- [Portable File Dialogs](https://github.com/samhocevar/portable-file-dialogs), alongside ImGui for native files dialogs
- [xdsopl's LDPC](https://github.com/xdsopl/LDPC), utilized for LDPC codes encoding/decoding
- [imgui_markdown](https://github.com/juliettef/imgui_markdown), to render this very file!

**Libraries included in plugins / Code taken from and in plugins :**
- Elektro/Arktika, [DecompWT](https://gitlab.eumetsat.int/open-source/PublicDecompWT), custom wavelet compression/decompression library originally used for MSG xRIT
- GK-2A, [libtomcrypt](https://github.com/libtom/libtomcrypt), for DES decryption
- GOES, [OpenJP2](https://github.com/uclouvain/openjpeg), for decompressiong J2K on GRB
- MetOp, [bzip2](https://github.com/libarchive/bzip2), for BZIP2 decompression utilized on MetOp admin messages
- Inmarsat, [libacars](https://github.com/szpajder/libacars), for ACARS parsing
- Inmarsat, [mbelib](https://github.com/szechyjs/mbelib), for AMBE audio decompression
- Inmarsat, [libaeroambe](https://github.com/jontio/libaeroambe), not the library itself, but the code was adapted (for Ambe decoding)
- Inmarsat, [Scytale-C](https://bitbucket.org/scytalec/scytalec), for STD-C packet formats and parsing

Those libraries above are included directly as they are either header-only, not already present on most systems or required some modifications for the purpose of this software. For the code included, the licenses of each respective library applies.

**Projects some code was taken from and included in libsatdump_core :**
- [LeanDVB](https://github.com/pabr/leansdr), for some definitions
- [gr-dvbs2rx](https://github.com/igorauad/gr-dvbs2rx), for TS Parsing and a few other bits
- [GNU Radio](https://github.com/gnuradio/gnuradio), for the convolutional decoding / encoding (quite heavily modified) and a few other bits
- [SDR++](https://github.com/AlexandreRouma/SDRPlusPlus), for the DSP stream implementation (thanks Ryzerth for the tip back then!) and a few other things, such as the SpyServer client

**Libraries linked against :**
- [Volk](https://github.com/gnuradio/volk), to simplify SIMD utilization
- [libpng](https://github.com/glennrp/libpng), for PNG image loading/saving
- [zlib](https://github.com/madler/zlib), required by libpng
- [nng](https://github.com/nanomsg/nng), for network stuff
- [fftw3](http://fftw.org/), used for all FFT operations
And additionally, on the UI side :
- [glew](http://glew.sourceforge.net/), for OpenGL loading
- [glfw3](https://www.glfw.org/), also for OpenGL

**SDR Libraries :**
- [libairspy](https://github.com/airspy/airspyone_host)
- [libairspyhf](https://github.com/airspy/airspyhf)
- [libhackrf](https://github.com/greatscottgadgets/hackrf)
- [LimeSuite](https://github.com/MyriadRF/LimeSuite)
- [librtlsdr](https://osmocom.org/projects/rtl-sdr/wiki)
- [libmirisdr4](https://github.com/f4exb/libmirisdr-4)
- [libsddc](https://github.com/ik1xpv/ExtIO_sddc)
- [libsdrplay](https://www.sdrplay.com/)

**Fonts :**
- [Roboto](https://fonts.google.com/specimen/Roboto), for text
- [3270 Nerd Font](https://www.nerdfonts.com/font-downloads), for icons and symbols  
*Both fonts were merge into a single font.ttf*

# Contributors

- Tomi HA6NAB
- Zbychu
- Arved MØKDS
- Ryzerth
- LazzSnazz
- Raov UB8QBD
- Peter Kooistra
- Felix OK9UWU
- Fred Jansen
- MeteoOleg
- Oleg Kutkov
- Mark Pentier
- Sam (@sam210723)
- Jpjonte
- Piefadase
- Blobtoe
- lego11
- crosswalkersam

# Thanks

- microp11 (Paul Maxan), for the reverse-engineering work & Scytale-C

**GNU**
- 
I'd just like to interject for a moment. What you're refering to as SatDump, is in fact, GNU/SatDump, or as I've recently taken to calling it, GNU plus SatDump...   
*Reader falls asleep*
