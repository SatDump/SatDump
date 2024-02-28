# Satellite pipelines

The aim of this is to list all possible satellite processing pipelines as well as their parameters. This does not explain the pipelines by any means. This should just simplify the CLIs use.

## NOAA

- `noaa_hrpt`: NOAA HRPT
- `noaa_gac`: NOAA GAC
  - `backward`: Over the US, NOAA satellites often transmit in reverse order. This mode allows decoding those backward replays properly.
- `noaa_dsb`: NOAA DSB
- `noaa_apt`: NOAA APT
  - `satellite_number`: For apt it is required to know what satellite is being received for projections and overlays. Options are `15, 18, 19`
  - `start_timestamp`: Required for projections and overlays. If your .wav file is a supported format it will be read automatically. Unix timestamp of the start of the file. Must be UTC Unix timestamp in seconds.
  - `autocrop_wedges`: This will automatically crop the image to only include telemetry wedges considered valid. May discard a lot on bad images!
  - `sdrpp_noise_reduction`: Uses the APT noise reduction originally implemented in SDR++. Defaults to true
  - `save_wav`: Saves the wav file. Defaults to false
  - `save_unsynced`: Saves the image before it's syncronized. Useful for weak signals. Defaults to true
 
## Meteor M

- `meteor_hrpt`: METEOR HRPT
  - `start_timestamp`: Unix timestamp of the start of the file provided. Must be UTC Unix timestamp in seconds.Required in case you are not processing your file on the same Mocow day.
- `meteor_m2_lrpt`: METEOR M2 LRPT 72k
  - `fill_missing`: Fills in black lines caused by signal drop-outs or interference
  - `max_fill_lines`: Maximum contiguous lines to correct. Default is 50
- `meteor_m2-x_lrpt`: METEOR M2-X LRPT 72k (Same parameters as meteor_m2_lrpt)
- `meteor_m2-x_lrpt_80k`: METEOR M2-x LRPT 80k (Same parameters as meteor_m2_lrpt)
- `meteor_m_dump_narrow`: METEOR-M Narrow Dump (WIP!)
- `meteor_m_dump_wide`: METEOR-M Wide Dump (WIP!)

## MetOp

- `metop_ahrpt`: MetOp AHRPT
  - `write_hpt`: Generate a .hpt file to load into HRPT Reader for processing.
- `metop_dump`: MetOp X-Band dump

## AIM

- `aim_dump`: AIM dump

## BlueWalker-3

- `bluewalker3_wide`: BlueWalker-3 S-Band Wide
- `bluewalker3_narrow`: BlueWalker-3 S-Band Narrow

## Cloudsat

- `cloudsat_link`: Cloudsat S-Band link

## Coriolis

- `coriolis_db`: Coriolis S-Band Tactical DB

## Cosmos

- `cosmos_2558_dump`: Cosmos 2558 S-Band dump

## Cryosat

- `cryosat_dump`: Cryosat X-Band dump

## DMSP

- `dmsp_rtd`: DMSP RTD
  - `satellite_number`: DMSP does not transmit a satellite ID. As instrument configurations do vary between them, it is required to identify the satellite. Options are `17, 18`.
 
## EOS

- `aqua_db`: Aqua DB
- `terra_db`: Terra DB
- `aura_db`: Aura DB

## Elektro / Arktika

- `elektro_rdas`: ELEKTRO-L RDAS
- `arktika_rdas`: ARKTIKA-M RDAS
- `elektro_lrit`: ELEKTRO_L LRIT
- `elektro_hrit`: ELEKTRO_L HRIT
- `elektro_tlm`: ELEKTRO_L TLM
- `arktika_tlm`: ARKTIKA-M L-Band TLM

## Fengyun-2

- `fengyun_svissr`: Fengyun-2 S-VISSR

## Fengyun-3

- `fengyun3_ab_hrpt`: FengYun-3 A/B AHRPT
  - `write_c10`: Generate a .C10 file to load into HRPT Reader for processing.
- `fengyun3_c_hrpt`: Fengyun_3 C HRPT
  - `write_c10`: Generate a .C10 file to load into HRPT Reader for processing.
- `fengyun3_abc_mpt`: FengYun-3 A/B/C MPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_d_ahrpt`: Fengyun-3 D AHRPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_e_ahrpt`: Fengyun-3 E AHRPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_g_ahrpt`: Fengyun-3 G AHRPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_abc_dpt`: Fengyun-3 A/B/C DPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_d_dpt`: Fengyun-3 D DPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_e_dpt`: Fengyun-3 E DPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_f_ahrpt`: FengYun-3 F AHRPT
  - `dump_mersi`: Dump raw MERSI frames for processing with other software, such as Fred's WeatherSat!
- `fengyun3_tlm_old`: FengYun-3 TLM (Old) A/B/C/D
- `fengyun3_tlm`: FengYun-3 TLM E/F
- 

## Fengyun-4

- `fengyun4_lrit`: Fengyun-4 LRIT
  - `ts_input`: Input TS instead of BBFrame
- `fengyun4_hrit23`: Fengyun-4 HRIT-II/III
  - `ts_input`: Input TS instead of BBFrame

## GCOM

- `gcom_w1_link`: GCOM-W1 link
- `gcom_c1_link`: GCOM-C1 link

## GEO-KOMPSAT_2A (GK-2A)

- `gk2a_lrit`: GK-2A LRIT
- `gk2a_lrit_tcp`: GK-2A LRIT to xrit-rx
- `gk2a_hrit`: GK-2A HRIT
- `gk2a_cdas`: GK-2A CDAS

## GOES

- `goes_gvar`: GOES GVAR
- `goes_hrit`: GOES-R HRIT
  - `write_dcs`: Save DCS LRIT files 
  - `write_lrit`: Write all LRIT files
- `goes_hrit_tcp`: GOES-R HRIT to goestools
- `goes_grb`: GOES-R GRB
- `goesr_cda`: GOES_R CDA
- `goes_md1`: GOES-N MDL
- `goes_lrit`: GOES-N LRIT
- `goesn_cda`: GOES-N CDA
- `goesn_sounder`: GOES-N Sounder SD
- `goesn_sd`: GOES-N Sounder Data
- `goesr_raw`: GOES-R Raw Data

## GeoNetCast

- `geonetcast`: GeoNetCast
  - `ts_input`: Input TS instead of BBFrame

## Himawari

- `himawaricast`: HimawariCast
  - `ts_input`: Input TS instead of BBFrame

## Inmarsat

- `inmarsat_std_c`: Inmarsat STD-C
- `inmarsat_aero_6`: Inmarsat Aero 0.6k (WIP)
- `inmarsat_aero_12`: Inmarsat Aero 1.2k (WIP)
- `inmarsat_aero_84`: Inmarsat Aero 8.4k
- `inmarsat_aero_105`: Inmarsat Aero 10.5k (WIP)

## JPSS

- `npp_hrd`: Suomi NPP / JPSS-1 HRD
- `jpss_hrd`: JPSS-2/3/4 HRD
- `jpss_tlm`: JPSS-2/3/4 Telemetry

## Jason-3

- `jason3_link`: Jason-3 S-Band link

## Lucky-7

- `lucky7_link`: Lucky-7 UHF link

## MATS

- `mats_dump`: MATS dump

## Oceansat

- `oceansat2_db`: OceanSat-2 DB
- `oceansat3_argos`: Oceansat-3 L-Band

## Orbcomm

- `orbcomm_stx`: Orbcomm STX

## Proba

- `proba1_dump`: Proba-1 dump
- `proba2_dump`: Proba-2 dump
- `probav_s_dump`: Proba-V S-Band dump
- `probav_x_dump`: Proba-V X-Band dump

## SpaceX

- `falcon9_tlm`: Falcon 9 S-Band TLM
- `starship_tlm`: Starship S-Band TLM
- `crew_dragon_tlm`: Crew Dragon S-Band TLM

## Stereo

- `stereo_lr`: Stereo-A/B Low Rate
- `stereo_hr`: Stereo-A/B High Rate

## TGO

- `tgo_link`: Mars TGO X-Band Link

## TUBSAT

- `tubin_x_dump`: TUBIN X-Band Dump
  - `check_crc`: Checks frames for errors. This is usually desireable, but sometimes ignoring errors may decode a bit more!

## UVSQ

- `inspiresat7_tlm`: INSPIRE-Sat7 TLM

## UmKA

- `umka_1_dump`: UmKA-1 dump

## Others

- `saral_l_band`: Salral L-Band
- `angels_l_band`: Angels L-Band
- `gazelle_l_band`: OTB-3/Gazelle L-Band
- `yunhai_ahrpt`: Yunhai AHRPT - Encrypted ;(
- `syracuse3b_tlm`: Syracuse 3B TLM
- `scisat1_dump`: SciSat-1 dump
- `CALIPSO`: Calipso S-Band dump
- `youthsat_dump`: YouthSat dump

## Chandrayaan

- `chandrayaan3_link_1k`: Chandrayaan-3 1k Link
- `chandrayaan3_link_2k`: Chandrayaan-3 2k Link
- `chandrayaan3_link_4k`: Chandrayaan-3 4k Link
- `chandrayaan3_link_8k`: Chandrayaan-3 8k Link

## DISCOVR

- `dscovr_tlm`: DSCOVR TLM Link
- `dscovr_hr`: DSCOVR High-Rate Link

## Hinode

- `hinode_s_dump`: Hinode S-Band Dump
- `hinode_s_tlm`: Hinode S-Band TLM

## Iris

- `iris_s_dump`: IRIS S-Band Dump
- `iris_dump`: IRIS X-Band Dump

## KPLO

- `kplo_sband_link`: KPLO (Danuri) S-Band Link

## Landsat

- `landsat_ldcm_tlm`: LandSat 8/9 S-band
- `landsat_ldcm_link`: LandSat 8/9 X-band

## Orion

- `orion_link`: Orion S-Band

## Sentinel-6

- `sentinel6_dump`: Sentinel-6 Dump
- `sentinel6_tlm`: Sentinel 6 S-Band TLM

## Tianwen

- `tianwen1_link`: Tianwen-1 Link

## ViaSat

- `viasat3_tlm`: ViaSat-3 TLM

## MSG

- `msg_raw`: MSG Raw Data

#

TODO: add Test, WIP
