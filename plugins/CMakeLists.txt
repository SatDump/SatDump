
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# We need those to be available for plugins!
include_directories(../src-core ../src-interface)

option(PLUGINS_ALL "Enable all plugins by default" ON)

option(PLUGIN_SPACEX "Enable the SpaceX support plugin" OFF)

if(PLUGIN_SPACEX OR PLUGINS_ALL)
    add_subdirectory(spacex_support)
endif()

option(PLUGIN_FY2 "Enable the FengYun-2 (S-VISSR, etc) support plugin" OFF)

if(PLUGIN_FY2 OR PLUGINS_ALL)
    add_subdirectory(fengyun2_support)
endif()

option(PLUGIN_FY3 "Enable the FengYun-3 (AHRPT, MPT, DPT, etc) support plugin" OFF)

if(PLUGIN_FY3 OR PLUGINS_ALL)
    add_subdirectory(fengyun3_support)
endif()

option(PLUGIN_FY4 "Enable the FengYun-4 (LRIT, HRIT) support plugin" OFF)

if(PLUGIN_FY4 OR PLUGINS_ALL)
    add_subdirectory(fengyun4_support)
endif()

option(PLUGIN_PROBA "Enable the Proba (1, 2, V, etc) support plugin" OFF)

if(PLUGIN_PROBA OR PLUGINS_ALL)
    add_subdirectory(proba_support)
endif()

option(PLUGIN_EOS "Enable the EOS (Aqua, Terra, Aura) support plugin" OFF)

if(PLUGIN_EOS OR PLUGINS_ALL)
    add_subdirectory(eos_support)
endif()

option(PLUGIN_METEOR "Enable the Meteor (HRPT, LRPT, etc) support plugin" OFF)

if(PLUGIN_METEOR OR PLUGINS_ALL)
    add_subdirectory(meteor_support)
endif()

option(PLUGIN_JPSS "Enable the JPSS (SNPP, JPSS-1, etc) support plugin" OFF)

if(PLUGIN_JPSS OR PLUGINS_ALL)
    add_subdirectory(jpss_support)
endif()

option(PLUGIN_GOES "Enable the GOES (GVAR, HRIT, GRB, etc) support plugin" OFF)

if(PLUGIN_GOES OR PLUGINS_ALL)
    add_subdirectory(goes_support)
endif()

option(PLUGIN_ELEKTRO_ARKTIKA "Enable the Elektro/Arktika (RDAS, xRIT, etc) support plugin" OFF)

if(PLUGIN_ELEKTRO_ARKTIKA OR PLUGINS_ALL)
    add_subdirectory(elektro_arktika_support)
endif()

option(PLUGIN_GK2A "Enable the GK2A (xRIT, etc) support plugin" OFF)

if(PLUGIN_GK2A OR PLUGINS_ALL)
    add_subdirectory(gk2a_support)
endif()

option(PLUGIN_NOAA_METOP "Enable the NOAA/MetOp POES (AHRPT, HRPT, X-Band dumps, GAC, etc) support plugin" OFF)

if(PLUGIN_NOAA_METOP OR PLUGINS_ALL)
    add_subdirectory(noaa_metop_support)
endif()

option(PLUGIN_DMSP "Enable the DMSP support plugin" OFF)

if(PLUGIN_DMSP OR PLUGINS_ALL)
    add_subdirectory(dmsp_support)
endif()

option(PLUGIN_JASON3 "Enable the Jason-3 (S-Band dumps) support plugin" OFF)

if(PLUGIN_JASON3 OR PLUGINS_ALL)
    add_subdirectory(jason3_support)
endif()

option(PLUGIN_AIM "Enable the AIM support plugin" OFF)

if(PLUGIN_AIM OR PLUGINS_ALL)
    add_subdirectory(aim_support)
endif()

option(PLUGIN_MATS "Enable the MATS support plugin" OFF)

if(PLUGIN_MATS OR PLUGINS_ALL)
    add_subdirectory(mats_support)
endif()

option(PLUGIN_OCEANSAT "Enable the Oceansat (X-Band, etc) support plugin" OFF)

if(PLUGIN_OCEANSAT OR PLUGINS_ALL)
    add_subdirectory(oceansat_support)
endif()

option(PLUGIN_HIMAWARI "Enable the Himawari (HimawariCast) support plugin" OFF)

if(PLUGIN_HIMAWARI OR PLUGINS_ALL)
    add_subdirectory(himawari_support)
endif()

option(PLUGIN_BLUEWALKER3 "Enable the BlueWalker-3 support plugin" OFF)

if(PLUGIN_BLUEWALKER3 OR PLUGINS_ALL)
    add_subdirectory(bluewalker3_support)
endif()

option(PLUGIN_ANALOG "Enable the support plugin for analog protocols such as NOAA APT" OFF)

if(PLUGIN_ANALOG OR PLUGINS_ALL)
    add_subdirectory(analog_support)
endif()

option(PLUGIN_ORBCOMM "Enable the support plugin for the Orbcomm constellation" OFF)

if(PLUGIN_ORBCOMM OR PLUGINS_ALL)
    add_subdirectory(orbcomm_support)
endif()

option(PLUGIN_GCOM "Enable the support plugin for the GCOM satellites" OFF)

if(PLUGIN_GCOM OR PLUGINS_ALL)
    add_subdirectory(gcom_support)
endif()

option(PLUGIN_TUBSAT "Enable the support plugin for the TUBSAT satellites (TUBIN, etc)" OFF)

if(PLUGIN_TUBSAT OR PLUGINS_ALL)
    add_subdirectory(tubsat_support)
endif()

option(PLUGIN_INMARSAT "Enable the support plugin for Inmarsat" OFF)

if(PLUGIN_INMARSAT OR PLUGINS_ALL)
    add_subdirectory(inmarsat_support)
endif()

option(PLUGIN_CUBESAT "Enable the support plugin for CubeSats" OFF)

if(PLUGIN_CUBESAT OR PLUGINS_ALL)
    add_subdirectory(cubesat_support)
endif()

option(PLUGIN_OTHERS "Enable the support plugin for a few other random satellites" OFF)

if(PLUGIN_OTHERS OR PLUGINS_ALL)
    add_subdirectory(others_support)
endif()

option(PLUGIN_DVB "Enable the support plugin for DVB Standards (DVB-S, DVB-S2)" OFF)

if(PLUGIN_DVB OR PLUGINS_ALL)
    add_subdirectory(dvb_support)
endif()

# add_subdirectory(sample)
option(PLUGIN_GVAR_EXTENDED "Enable the GVAR Extended plugin" OFF)

if(PLUGIN_GVAR_EXTENDED)
    add_subdirectory(gvar_extended)
endif()

# ######################
option(PLUGIN_SIMD_SSE41 "Enable SIMD SSE41 plugin" ON)

if(PLUGIN_SIMD_SSE41)
    add_subdirectory(simd_extensions/simd_sse41)
endif()

option(PLUGIN_SIMD_NEON "Enable SIMD NEON plugin" ON)

if(PLUGIN_SIMD_NEON)
    add_subdirectory(simd_extensions/simd_neon)
endif()

option(PLUGIN_SIMD_AVX2 "Enable SIMD AVX2 plugin" ON)

if(PLUGIN_SIMD_AVX2)
    add_subdirectory(simd_extensions/simd_avx2)
endif()

# ######################
option(PLUGIN_AIRSPY_SDR_SUPPORT "Enable Airspy SDR Support plugin" ON)

if(PLUGIN_AIRSPY_SDR_SUPPORT)
    add_subdirectory(sdr_sources/airspy_sdr_support)
endif()

option(PLUGIN_AIRSPYHF_SDR_SUPPORT "Enable AirspyHF SDR Support plugin" ON)

if(PLUGIN_AIRSPYHF_SDR_SUPPORT)
    add_subdirectory(sdr_sources/airspyhf_sdr_support)
endif()

option(PLUGIN_HACKRF_SDR_SUPPORT "Enable HackRF SDR Support plugin" ON)

if(PLUGIN_HACKRF_SDR_SUPPORT)
    add_subdirectory(sdr_sources/hackrf_sdr_support)
endif()

option(PLUGIN_LIMESDR_SDR_SUPPORT "Enable LimeSDR SDR Support plugin" ON)

if(PLUGIN_LIMESDR_SDR_SUPPORT)
    add_subdirectory(sdr_sources/limesdr_sdr_support)
endif()

option(PLUGIN_SDRPLAY_SDR_SUPPORT "Enable SDRPlay SDR Support plugin" ON)

if(PLUGIN_SDRPLAY_SDR_SUPPORT)
    add_subdirectory(sdr_sources/sdrplay_sdr_support)
endif()

option(PLUGIN_PLUTOSDR_SDR_SUPPORT "Enable PlutoSDR SDR Support plugin" ON)

if(PLUGIN_PLUTOSDR_SDR_SUPPORT)
    add_subdirectory(sdr_sources/plutosdr_sdr_support)
endif()

option(PLUGIN_BLADERF_SDR_SUPPORT "Enable BladeRF SDR Support plugin" ON)

if(PLUGIN_BLADERF_SDR_SUPPORT)
    add_subdirectory(sdr_sources/bladerf_sdr_support)
endif()

option(PLUGIN_USRP_SDR_SUPPORT "Enable USRP SDR Support plugin" ON)

if(PLUGIN_USRP_SDR_SUPPORT)
    add_subdirectory(sdr_sources/usrp_sdr_support)
endif()

option(PLUGIN_RTLSDR_SDR_SUPPORT "Enable RTL-SDR SDR Support plugin" ON)

if(PLUGIN_RTLSDR_SDR_SUPPORT)
    add_subdirectory(sdr_sources/rtlsdr_sdr_support)
endif()

option(PLUGIN_MIRISDR_SDR_SUPPORT "Enable MiriSDR SDR Support plugin" ON)

if(PLUGIN_MIRISDR_SDR_SUPPORT)
    add_subdirectory(sdr_sources/mirisdr_sdr_support)
endif()

option(PLUGIN_SPYSERVER_SUPPORT "Enable SpyServer Support plugin" ON)

if(PLUGIN_SPYSERVER_SUPPORT)
    add_subdirectory(sdr_sources/spyserver_support)
endif()

option(PLUGIN_RTLTCP_SUPPORT "Enable RTL-TCP Support plugin" ON)

if(PLUGIN_RTLTCP_SUPPORT)
    add_subdirectory(sdr_sources/rtltcp_support)
endif()

option(PLUGIN_SDRPP_SERVER_SUPPORT "Enable SDR++ Server Support plugin" ON)

if(PLUGIN_SDRPP_SERVER_SUPPORT)
    add_subdirectory(sdr_sources/sdrpp_server_support)
endif()

option(PLUGIN_AARONIA_SDR_SUPPORT "Enable Aaronia Spectran Support plugin" OFF)

if(PLUGIN_AARONIA_SDR_SUPPORT)
    add_subdirectory(sdr_sources/aaronia_sdr_support)
endif()

option(PLUGIN_RTAUDIO_SDR_SUPPORT "Enable RtAudio Source Support plugin" OFF)

if(PLUGIN_RTAUDIO_SDR_SUPPORT)
    add_subdirectory(sdr_sources/rtaudio_sdr_support)
endif()

# Disable by default, sort of experimental
option(PLUGIN_SDDC_SDR_SUPPORT "Enable SDDC (RX666, RX888, etc) SDR Support plugin" OFF)

if(PLUGIN_SDDC_SDR_SUPPORT)
    add_subdirectory(sdr_sources/sddc_sdr_support)
endif()

# ######################
option(PLUGIN_RTAUDIO_SINK "Enable RtAudio Audio Sink plugin" ON)

if(PLUGIN_RTAUDIO_SINK)
    add_subdirectory(audio_sinks/rtaudio_audio_sink)
endif()

option(PLUGIN_PORTAUDIO_SINK "Enable PortAudio Audio Sink plugin" ON)

if(PLUGIN_PORTAUDIO_SINK)
    add_subdirectory(audio_sinks/portaudio_audio_sink)
endif()

option(PLUGIN_AAUDIO_SINK "Enable AAudio Audio Sink plugin" ON)

if(PLUGIN_AAUDIO_SINK)
    add_subdirectory(audio_sinks/aaudio_audio_sink)
endif()