# SatDump

First time here? See the reference documents below to get started using SatDump.
[About](https://www.satdump.org/about/) | [Basic Usage](https://www.satdump.org/posts/basic-usage/) | [Docs](https://docs.satdump.org/)

# Libraries

**Libraries included in libsatdump_core**
- [bzip2](https://github.com/libarchive/bzip2), for BZIP2 decompression utilized on MetOp admin messages and Himawaricast
- [ctpl](https://github.com/vit-vit/ctpl), for thread pools used over the program
- [deepspace-turbo](https://github.com/geeanlooca/deepspace-turbo), used for Turbo decoding
- [dlfcn-win32](https://github.com/dlfcn-win32/dlfcn-win32), to use the usual dlopen() functions on Windows
- [ImGui](https://github.com/ocornut/imgui), for the user interface
- [imgui_markdown](https://github.com/juliettef/imgui_markdown), to render composite info popups and this very file!
- [implot](https://github.com/epezent/implot), to display images in the viewer
- [libaec](https://gitlab.dkrz.de/k202009/libaec), with OpenSatelliteProject's path
- [libcorrect](https://github.com/quiet/libcorrect), for Reed-Solomon decoding
- [libjpeg](https://ijg.org/), from the Independent JPEG Group
- [libOpenCL-loader](https://github.com/robertwgh/libOpenCL-loader), for OpenCL on Android
- [libpredict](https://github.com/la1k/libpredict), used for orbit prediction
- [Lua](https://www.lua.org/), used for complex image composites
- [miniz](https://github.com/richgel999/miniz), used to decompress ZIP files in some decoders
- [MuParser](https://github.com/beltoforion/muparser), for expression parsing (such as in composites)
- [Nlhohmann's JSON](https://github.com/nlohmann/json), for JSON and CBOR encoding/parsing
- [OpenCL C++ Headers](https://github.com/KhronosGroup/OpenCL-CLHPP), for OpenCL support
- [OpenJP2](https://github.com/uclouvain/openjpeg), for JPEG-2000 support on GOES GRB, FY4, and more
- [Portable File Dialogs](https://github.com/samhocevar/portable-file-dialogs), for native files dialogs
- [QOI](https://github.com/phoboslab/qoi), for qoi image support
- [RapidXML](http://rapidxml.sourceforge.net/), for XML Parsing
- [sol2](https://github.com/ThePhD/sol2), C++ Bindings for LUA
- [UTF-8 CPP](https://utfcpp.sourceforge.net/), for UTF-8 handling
- [xdsopl's LDPC](https://github.com/xdsopl/LDPC), utilized for LDPC codes encoding/decoding

### Libraries included in plugins / Code taken from and in plugins
- Elektro/Arktika, [DecompWT](https://gitlab.eumetsat.int/open-source/PublicDecompWT), custom wavelet compression/decompression library originally used for MSG xRIT
- GK-2A, [libtomcrypt](https://github.com/libtom/libtomcrypt), for DES decryption
- Inmarsat, [libacars](https://github.com/szpajder/libacars), for ACARS parsing
- Inmarsat, [mbelib](https://github.com/szechyjs/mbelib), for AMBE audio decompression
- Inmarsat, [libaeroambe](https://github.com/jontio/libaeroambe), not the library itself, but the code was adapted (for Ambe decoding)
- Inmarsat, [Scytale-C](https://bitbucket.org/scytalec/scytalec), for STD-C packet formats and parsing

*Those libraries above are included directly as they are either header-only, not already present on most systems or required some modifications for the purpose of this software. For the code included, the licenses of each respective library applies.*

### Projects some code was taken from and included in libsatdump_core
- [GNU Radio](https://github.com/gnuradio/gnuradio), for the convolutional decoding / encoding (quite heavily modified) and a few other bits
- [gr-dvbs2rx](https://github.com/igorauad/gr-dvbs2rx), for TS Parsing and a few other bits
- [LeanDVB](https://github.com/pabr/leansdr), for some definitions
- [SDR++](https://github.com/AlexandreRouma/SDRPlusPlus), for the DSP stream implementation (thanks Ryzerth for the tip back then!) and a few other things, such as the SpyServer client

### Libraries linked against
- [curl](https://curl.se/), for HTTP(S) support
- [fftw3](http://fftw.org/), used for all FFT operations
- [hdf5](https://github.com/HDFGroup/hdf5), for supporting official products and services like GeoNetCast
- [jemalloc](https://jemalloc.net/), for memory allocation optimization on Linux and macOS
- [libpng](https://github.com/glennrp/libpng), for PNG image loading/saving
- [nng](https://github.com/nanomsg/nng), for network stuff
- [PortAudio](https://www.portaudio.com/), used for audio output
- [Volk](https://github.com/gnuradio/volk), to simplify SIMD utilization
- [zlib](https://github.com/madler/zlib), required by libpng

### SDR Libraries
- [Aaronia](https://aaronia.com/en/support/downloads#rtsa-suite)
- [libairspy](https://github.com/airspy/airspyone_host)
- [libairspyhf](https://github.com/airspy/airspyhf)
- [libbladerf](https://github.com/Nuand/bladeRF/)
- [libhackrf](https://github.com/greatscottgadgets/hackrf)
- [libiio](https://github.com/analogdevicesinc/libiio) and [libad9361](https://github.com/analogdevicesinc/libad9361-iio) for PlutoSDR
- [libmirisdr4](https://github.com/f4exb/libmirisdr-4)
- [librtlsdr](https://osmocom.org/projects/rtl-sdr/wiki)
- [libsddc](https://github.com/ik1xpv/ExtIO_sddc)
- [libsdrplay](https://www.sdrplay.com/)
- [LimeSuite](https://github.com/MyriadRF/LimeSuite)
- [UHD](https://github.com/EttusResearch/uhd)

### UI Libraries
- [OpenGL ES](https://www.khronos.org/opengles/) and [EGL](https://www.khronos.org/egl), for OpenGL on Android
- [gl3w](https://github.com/skaslev/gl3w) and [glfw3](https://www.glfw.org/), for OpenGL on Desktop

### Fonts
- [Roboto](https://fonts.google.com/specimen/Roboto), for default font
- [3270 Nerd Font](https://www.nerdfonts.com/font-downloads), for icons and symbols
- [Liberation Serif](https://www.fontsquirrel.com/fonts/liberation-serif), for Solaris Motif theme
- [Perfect DOS VGA 437](https://www.dafont.com/perfect-dos-vga-437.font), for Phosphor theme
- [PX Sans Nouveaux](https://www.dafont.com/px-sans-nouveaux.font), for Windows 98 theme
*3270 Nerd Font and Roboto have been merged into a single font.ttf*

# Developers
**Lead Developer:** Aang23 (F4LAU)

### Additional Developers
- crosswalkersam
- Jamie Vital (KC3TWZ)
- lego11 (IU1QPT)
- Zbychu (SP5EWS)

### Contributors
- Arved MØKDS
- Blobtoe
- CO2ESP
- Daniel Ekman (SA2KNG)
- Digitelektro
- Felix OK9UWU
- Fred Jansen
- Jpjonte
- LazzSnazz
- Mark Pentier
- MeteoOleg
- Oleg Kutkov
- Peter Kooistra
- Piefadase
- Ryzerth
- Raov UB8QBD
- Sam (@sam210723)
- Scott Tilley (VE7TIL)
- Tomi HA6NAB

### Special Thanks to
- microp11 (Paul Maxan), for the reverse-engineering work & Scytale-C

### GNU
I'd just like to interject for a moment. What you're refering to as SatDump, is in fact, GNU/SatDump, or as I've recently taken to calling it, GNU plus SatDump...   
*Reader falls asleep*
