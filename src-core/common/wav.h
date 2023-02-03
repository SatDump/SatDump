#pragma once

#include <cstdint>
#include <string>
#include <ctime>

/*
Simple class to read a WAV header,
meant to read out values like samplerate, sample format, etc
This is not meant to parse everything not the actual sample
data.
*/
namespace wav
{
#ifdef _WIN32
#pragma pack(push, 1)
#endif
    struct WavHeader
    {
        char RIFF[4];             // RIFF Header
        uint32_t chunk_size;      // RIFF Chunk Size
        char WAVE[4];             // WAVE Header
        char fmt[4];              // FMT Header
        uint32_t sub_chunk_size;  // FMT Chunk size. 16 = PCM, 18 = IEEE Float, 40 = Extensible
        uint16_t audio_Format;    // Audio format. 1 = PCM, 3 = IEEE Float, 6 = mulaw, 7 = alaw, 257 = IBM Mu-Law, 258 = IBM A-Law, 259 = ADPCM, 65534 = Extensible
        uint16_t channel_cnt;     // Channel number 1 = Mono, 2 = Stereo
        uint32_t samplerate;      // Samplerate in Hz
        uint32_t bytes_per_sec;   // Bytes / second
        uint16_t block_align;     // 2 = 16-bit mono, 4 = 16-bit stereo, 6 = 24-bit stereo, 8 = 32-bit stereo
        uint16_t bits_per_sample; // Bits per sample
        char data[4];             // Data header
        uint32_t dw_data_length;  // Raw lengths
    }
#ifdef _WIN32
    ;
#else
    __attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
    struct RF64Header
    {
        char RIFF[4];             // RIFF Header
        uint32_t chunk_size;      // Unused
        char WAVE[4];             // WAVE Header
        char fill[36];            // Ignore this
        char fmt[4];              // FMT Header
        uint32_t sub_chunk_size;  // FMT Chunk size. 16 = PCM, 18 = IEEE Float, 40 = Extensible
        uint16_t audio_Format;    // Audio format. 1 = PCM, 3 = IEEE Float, 6 = mulaw, 7 = alaw, 257 = IBM Mu-Law, 258 = IBM A-Law, 259 = ADPCM, 65534 = Extensible
        uint16_t channel_cnt;     // Channel number 1 = Mono, 2 = Stereo
        uint32_t samplerate;      // Samplerate in Hz
        uint32_t bytes_per_sec;   // Bytes / second
        uint16_t block_align;     // 2 = 16-bit mono, 4 = 16-bit stereo, 6 = 24-bit stereo, 8 = 32-bit stereo
        uint16_t bits_per_sample; // Bits per sample
        char data[4];             // Data header
        uint32_t dw_data_length;  // Raw lengths
    }
#ifdef _WIN32
    ;
#else
    __attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

    WavHeader parseHeaderFromFileWav(std::string file);
    RF64Header parseHeaderFromFileRF64(std::string file);
    bool isValidWav(WavHeader header);
    bool isValidRF64(WavHeader header);

    // Filenames
    struct FileMetadata
    {
        uint64_t frequency = 0;
        time_t timestamp = 0;
    };

    FileMetadata tryParseFilenameMetadata(std::string filepath, bool audio = false);
};
