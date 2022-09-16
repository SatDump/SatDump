# ZIQ (Baseband Format)

In SatDump, a non-standard baseband format is available : ZIQ. The main difference compared to other raw formats is that ZIQ support compressed the raw samples to save on disk space.

As many usecases of SatDump may require recording rather large basebands (easily 100GB+ on X-Band!), experimentation was done using several algorithms to see what could be done. ZSTD revealed rather good compression ratios (often 40% or more at 8-bits) with a minimal performance impact. As it ended up being extremely useful, it was decided to turn it into something easier to use, hence carrying an information header and more as a WAV file would, and that's how ZIQ became a thing (ZIQ because it uses the Z-Standard, and IQ as it is used for storing complex IQ basebands).

The full ZIQ implementation (C++) can be found in src-core/common/ziq.h and src-core/common/ziq.cpp. Anyone is free to reuse it for implementation in their project. Do keep in mind this implementation uses VOLK for SIMD acceleration, so you may have to change that before use in your own project.

## Header

The ZIQ header is formatted as follow :

|        Value        |         Descritpion           |  Size   |   Type   |
|---------------------|-------------------------------|---------|----------|
| `ZIQ_`              | File signature                | 4-bytes | char[4]  |
| `is_compressed`     | Compression flag              | 1-byte  | bool     |
| `bits_per_sample`   | Bits per sample               | 1-byte  | uint8_t  |
| `samplerate`        | Samplerate                    | 8-bytes | uint64_t |
| `annotation_length` | Length of the annotation JSON | 8-bytes | uint64_t |
| `annotation_str`    | Annotation string             | Varies  | char[]   |

File signature :  
*Used to identify a ZIQ file, again as it would be done in a WAV file.*  

Compression flag :  
*ZIQ files can be compressed or uncompressed. If this flag is set, the file is using ZSTD compression, otherwise raw samples are utilized.*

Bits per sample :  
*This field indicated the size of the samples utilized in the payload area. They can be either :*
- *8-bits, where the int8_t type should be used*
- *16-bits, where int16_t should be used, in little endian*
- *32-bits, which means 32-bits IEEE floats. Again, in little endian*

Samplerate :  
*This is simply the complex sampling rate of the contained IQ stream.*

Annotation fields :  
*ZIQ can also hold a JSON string in the header to describe additional parameters. The annotation length field should indicate the length of the contained ASCII string.*

## Payload

After the header, the raw compressed or uncompressed stream is simply appended directly. It's not stored in chunks as ZSTD can automatically "synchronize" on a stream in case something was to be corrupted.

## Future considerations

In the future, it could be interested for ZIQ to support more various bit depths and sample formats. Perhaps other compression algorithms aiming at very high compression ratios (perhaps lossy?) could be considered, hence modifying the compression flag to support several values instead of a simple bool. If this happens, 0 would stay the default for no compression, and 1 for ZSTD compression for retro-compatbility.