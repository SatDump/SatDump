# What products need to do

## General definition

Products are defined (in the context of SatDump) as processed instrument raw data, carrying additional metadata to allow calibration and geo-referencing routines.

## Data types / Storage method

Products should be able to accomodate any sort of data expected on satellite data that should be handled in product form. This does not concern things like video (Eg, Falcon-9) or Admin messages which would not benefit from further processing.  

Hence, the main data types required are :
- Images, which should be stored in standard formats (eg, PNG) for user convenience and referenced in the product's data to be loaded on opening it. Images mainly concern radiometer and spectrometer data, would it be UV, microwave or Visible/IR as well as sun imagers.   
It is necessary to handle both instruments that continuously scan and those that take a single "picture". Though, those could likely be split into several sub-products.  
Timestamps, either per scanline (AVHRR, etc), per group of scanlines (MERSI, VIIRS, which scans 10 or 40 at once), or IFOV ("square of the image") for IASI, MODIS, etc. should be supported, alongside a way to fetch the kepler data to use (TLEs) and projections settings.   
As far as calibration goes, it could either be done before writing the products out, only providing the scale to use for converting back to physical units, or dropping the calibration data in the metadata as well as the formula to use.   
The instrument type and extra information should be put into the products.
- Radiation data, and other similar data such as space debris counters. Those only require basic metadata such as timestamps. The actual counts should either be held in a separate file or in the product file itself.
- Altimeter data, either in SAR mode (Cryosat, Sentinel-6, etc) and single pulse mode (Jason-3, etc). In this instance, it is still yet to be determined if storing the raw IQ of each echo is preferred, or simply processing height or other data.  
In all cases though, timestamp storage alongside TLEs is the main requirement.
- Scaterometer data, which for ASCAT or similar are stored as float which cannot be stored in a PNG. The requirements are the same as for images, with data stored directly in the product. Loading from images should still be supported as some instruments do utilize "normal" bit depths.
- Random, high bit-depth data such as microwave column sounders and so, which are not images, closer to radiation data but need to store higher fidelity data and processed differently. This concerns things such as SBUV, CPR on CloudSat (which needs to be plotted vertically). Requirements again include timestamps and various custom metadata which may be required.

In summary : the product formats should be able to accomodate any sort of data, which spans over a wide range covering imagery, counts / single mesurements, 3D elevation data processed from raw IQ and other random formats requiring. Data should either be stored in the main file or loaded from external, standard formats. 

## Proposed solution

As the main product file needs to store various data types in a structured, varying manner, the flexibility of a JSON-style format would be perfect.   
Unfortunately, JSON while practical is text-based, resulting in huge filesizes for any decent amount of data. In this instance, human-readability is not the main concern so an equivalent binary-based format is preferred.  
Personally, I found CBOR, which results is much lower filesizes and is already supported by the JSON library used in SatDump to be a viable option.

A question I have pending though is : should it be compressed? Compressing a file storing a few timestamps results in over 70% of disk space savings in many cases, but that's at the cost of standardization. (CBOR by itself is standard, while added compression would require anyone implementing this to support the compression aspect on their end).