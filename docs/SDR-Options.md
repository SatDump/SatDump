# SDR Options

This aims at covering all available SDR Settings for supported devices / sources. The goal is not to explain what every option does (for that, see the manufacturer's documentation for the specific device), but rather allow users to find the right flags in CLI mode.  

Where possible, consistency was kept to be rather easy to "guess" if you know what you are looking for.

## Airspy

- `gain_type` : The gain mode used by the device :
    - 0 is for Sensitive
    - 1 is for Linear
    - 2 is for Manual
- `general_gain` : Overall gain for sensitive and linear mode. Ranges from 0 to 21, in dBs
- `lna_gain, mixer_gain, vga_gain` : Used for manual gain tuning only
- `bias` : Enable Bias-Tee power
- `lna_agc` : Enable the LNA AGC
- `mixer_agc` : Enable the Mixer AGC

## AirspyHF

- `agc_mode` : 0 for disabled, otherwise :
    - 1 for LOW
    - 2 for HIGH
- `attenuation` : Attenuation in dBs
- `hf_lna` : Enable or disable the HF LNA

## HackRF

- `amp` : Enable the main (non-programmable) amplifier
- `lna_gain` : LNA Gain in dBs
- `vga_gain` : VGA Gain in dBs
- `bias` : Enable Bias-Tee power

## BladeRF

- `gain_mode` : 
    - 0 is device default
    - 1 is manual
    - 2 is fast AGC
    - 3 is slow AGC
    - 4 is hybrid AGC
- `gain` : General Gain in dBs
- `bias` : Bias-Tee power (BladeRF 2.0 only)

## LimeSDR

- `gain` : Gain in dBs

## RTL-SDR

- `gain` : Device Gain in dBs
- `agc` : Enable or disable the AGC
- `bias` : Enable Bias-Tee power

## RTL-TCP

- `ip_address` : IPv4 Server address
- `port` : Server port. Usually 1234
- `gain` : Device Gain in dBs
- `lna_agc` : Enable or disable the built-in LNA AGC
- `bias` : Enable Bias-Tee power (if supported)

## Aaronia Spectran V6

- `ref_level` : Reference Level, in dBs
- `usb_compression` : 
    - 0 is auto
    - 1 is raw
    - 2 is compressed
- `agc_mode` : 
    - 0 is manual
    - 1 is peak
    - 2 is power
- `enable_amp` : Enable the amp
- `enable_preamp` : Enable the preamp

## SDDC (RX888, RX999, etc)

*Note : Support for those is experimental. Things may not work as expected!*

- `mode` : 0 for HF, 1 for VHF
- `rf_gain` : RF Gain in dBs
- `if_gain` : IF Gain in dBs
- `bias` : Enable Bias-Tee power

## SDRPlay

*Note : some options are device-specific!*

- `lna_gain` : LNA Gain in dBs
- `if_gain` : IF Gain in dBs
- `bias` : Enable Bias-Tee power
- `am_notch` : Enable the AM notch filter
- `fm_notch` : Enable the FM notch filter
- `dab_notch` : Enable the DAB notch filter
- `am_port` : Select the AM antenna port
- `antenna_input` : Select a specific antenna input. 0 is the first input
- `agc_mode` : AGC Mode, 0 is disabled :
    - 1 is 5Hz
    - 2 is 50Hz
    - 3 is 500Hz

## SpyServer

- `ip_address` : IPv4 Server address
- `port` : Server port. Usually 5555
- `bit_depth` : Bit depth to stream at. Options are 8/16/32
- `gain` : Device gain in dBs
- `digital_gain` : Software gain, in dBs

## SDR++ Server

*Note : Using SDR++ in CLI mode as a source is possible, but all settings have to be setup from an UI connecting to the server beforehand, including samplerate!*

- `ip_address` : IPv4 Server address
- `port` : Server port. Usually 5259
- `bit_depth` : Bit depth to stream at. Options are 8/16/32
- `compression` : Use SDR++ Server compression (true / false)

## PlutoSDR

- `gain` : Device Gain in dBs
- `gain_mode` : Gain mode :
    - 1 is Manual
    - 2 is Fast Attack
    - 3 is Slow Attack
    - 4 is Hybrid

## USRP

- `gain` : Device Gain in dBs
- `channel` : Channel ID
- `antenna` : Antenna ID
- `bit_depth` : Bit depth. Can be 16 on all devices, 8 or 12 on some

## MiriSDR

- `gain` : Device Gain in dBs
- `bias` : Enable Bias-Tee power
