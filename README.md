This is a fork of SatDump with a few modifications to make it easier to use when decoding STD-C and ACARS messages using the CLI interface.  

The changes in this fork include:  
- SSU and AES System Table Broadcast Index messages are neither saved to file nor printed to the terminal log


Here is a copy of the json file I use to grab all of the STD-C and AERO 10500k channels from Inmarsat 4F3 (98W):

```
{
	"vfo0": {
                "frequency": 1537700000,
                "pipeline": "inmarsat_std_c"
        },

	"vfo1": {
                "frequency": 1539525000,
                "pipeline": "inmarsat_std_c"
        },

	"vfo2": {
                "frequency": 1539545000,
                "pipeline": "inmarsat_std_c"
        },

	"vfo3": {
                "frequency": 1539555000,
                "pipeline": "inmarsat_std_c"
        },

	"vfo4": {
                "frequency": 1539565000,
                "pipeline": "inmarsat_std_c"
        },

	"vfo5": {
                "frequency": 1539585000,
                "pipeline": "inmarsat_std_c"
        },

	"vfo6": {
                "frequency": 1539685000,
                "pipeline": "inmarsat_std_c"
        },

	"vfo7": {
                "frequency": 1546006000,
                "pipeline": "inmarsat_aero_105"
        },
	"vfo8": {
                "frequency": 1546021000,
                "pipeline": "inmarsat_aero_105"
        },

	"vfo9": {
                "frequency": 1546063500,
                "pipeline": "inmarsat_aero_105"
        },

	"vfo10": {
                "frequency": 1546078500,
                "pipeline": "inmarsat_aero_105"
        }

}

```

Here is the command I use to run this fork:
```
./satdump live inmarsat_aero_105 /home/user/satdump/VFO/20241222 --source airspy --samplerate 10e6 --frequency 1542160000 --bias --gain_type 0 --general_gain 21 --multi_vfo /home/user/satdump/98W.json
```
