# Official Products

SatDump supports decoding and processing a select portfolio of official products from space and meteorological agencies.

> [!note]
> This module is experimental and could contain bugs; users are encouraged to test and report any issues [on the GitHub](https://github.com/SatDump/SatDump/issues).

All products must be decoded with the `Data Stores/Archives Formats (NOAA/EUMETSAT/CMA/NASA), Experimental` pipeline.

## List of supported products

### EUMETSAT

| Product                                                           | Satellite  | Instrument | Link                                                           | Notes |
|-------------------------------------------------------------------|------------|------------|----------------------------------------------------------------|-------|
| High Rate SEVIRI Level 1.5 Image Data - MSG - 0 degree            | MSG        | SEVIRI     | https://data.eumetsat.int/product/EO:EUM:DAT:MSG:HRSEVIRI      |       |
| High Rate SEVIRI Level 1.5 Image Data - MSG - Indian Ocean        | MSG        | SEVIRI     | https://data.eumetsat.int/product/EO:EUM:DAT:MSG:HRSEVIRI-IODC |       |
| Rapid Scan High Rate SEVIRI Level 1.5 Image Data - MSG            | MSG        | SEVIRI     | https://data.eumetsat.int/product/EO:EUM:DAT:MSG:MSG15-RSS     |       |
| FCI Level 1c Normal Resolution Image Data - MTG - 0 degree        | MTG        | FCI        | https://data.eumetsat.int/product/EO:EUM:DAT:0662              |       |
| FCI Level 1c High Resolution Image Data - MTG - 0 degree          | MTG        | FCI        | https://data.eumetsat.int/product/EO:EUM:DAT:0665              |       |
| AVHRR Level 1B - Metop - Global                                   | Metop      | AVHRR      | https://data.eumetsat.int/product/EO:EUM:DAT:METOP:AVHRRL1     |       |
| IASI Level 1C - All Spectral Samples - Metop - Global             | Metop      | IASI       | https://data.eumetsat.int/product/EO:EUM:DAT:METOP:IASIL1C-ALL |       |
| AMSU-A Level 1B - Metop - Global                                  | Metop      | AMSU-A     | https://data.eumetsat.int/product/EO:EUM:DAT:METOP:AMSUL1      |       |
| MHS Level 1B - Metop - Global                                     | Metop      | MHS        | https://data.eumetsat.int/product/EO:EUM:DAT:METOP:MHSL1       |       |
| OLCI Level 1B Full Resolution - Sentinel-3                        | Sentinel-3 | OLCI       | https://data.eumetsat.int/product/EO:EUM:DAT:0409              |       |
| SLSTR Level 1B Radiances and Brightness Temperatures - Sentinel-3 | Sentinel-3 | SLSTR      | https://data.eumetsat.int/data/map/EO:EUM:DAT:0411             |       |