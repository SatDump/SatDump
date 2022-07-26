# Plugin List

Since SatDump 1.0.0, most if not all code that is not common between many satellites or features is not in the main library anymore, but instead in plugins. Those plugins are entirely optional, and there is no requirement anymore to build the entire codebase.

By default `-DPLUGINS_ALL=ON` is set in CMake, but setting it to `OFF` will allow selecting what you wish to build. The syntax to enable a specific plugin is `-DPLUGIN_NAME=ON`.

The list provided belows aims at being a "quick-look" of what every plugin covers to help unfamiliar users decide on what they actually need.

The string in paranthesis after each plugin is the argument to be passed to CMake.

# Satellite support

## Elektro/Arktika (PLUGIN_ELEKTRO_ARKTIKA)

This plugin cover the ELEKTRO and ARKTIKA Russian weather satellites, both being very similar.

Supported downlinks :
- X-Band RDAS from both
- ELEKTRO-L LRIT
- ELEKTRO-L HRIT
- Low-Rate radiation payload downlink

## EOS (PLUGIN_EOS)

This covers NASA's Earth Observation System.
- Aqua (Direct Broadcast)
- Terra (Direct Broadcast)
- Aura (Direct Broadcast)

## FengYun-2 (PLUGIN_FY2)

Covers the first generation of FengYun GEO sats. Only S-VISSR is supported.

## FengYun-3 (PLUGIN_FY3)

Supports all FengYun-3 satellites :
- FengYun-A/B/C AHRPT on L-Band
- FengYun-A/B/C MPT on X-Band
- FengYun-A/B X-Band dumps (DPT)
- FengYun-D/E X-Band AHRPT

## GK-2A (PLUGIN_GK2A)

Supported downlinks :
- GK-2A LRIT
- GK-2A HRIT

## GOES (PLUGIN_GOES)

This covers the NOAA GOES weather satellites :
- GOES-N GVAR
- GOES-R HRIT
- GOES-R CDA
- GOES-R CDA

## Himawari (PLUGIN_HIMAWARI)

This provides support for the HimawariCast broadcast.

## Jason-3 (PLUGIN_JASON3)

Supports the Jason-3 Altimetry mission on S-Band.

## JPSS (PLUGIN_JPSS)

Provides support for the JPSS satellite mission :
- Suomi NPP and JPSS-1 HRD
- JPSS-2/3/4 HRD

## Meteor (PLUGIN_METEOR)

Covers the russian METEOR missions :
- L-Band AHRPT
- LRPT on VHF

## NOAA/MetOp (PLUGIN_NOAA_METOP)

This covers the POES missions, including the first generation MetOps and NOAA POES, carrying very similar instruments.

Support downlinks :
- NOAA DSB
- NOAA HRPT
- NOAA GAC
- MetOp AHRPT
- MetOp X-Band dumps

## OceanSat (PLUGIN_OCEANSAT)

Provides support for the OceanSat Indian satellite missions.
- OceanSat-2 Direct Broadcast

## Others (PLUGIN_OTHERS)

This plugin acts as a temporary place before some satellites are moved into their own plugin.  
Currently, this includes :
- Angels
- CloudSat
- Coriolis
- Saral

## Proba (PLUGIN_PROBA)

Covers the ESA Project for OnBoard Autonomy series of satellites :
- Proba-1
- Proba-2
- Proba-V

## SpaceX (PLUGIN_SPACEX)

This contains old support for the SpaceX Falcon-9 and StarShip telemetry downlinks. This is currently not updated as SpaceX has made the decision to encrypt the telemetry since then, but the code is kept to allow processing older data and in the eventuallity encryption is turned off in the future.