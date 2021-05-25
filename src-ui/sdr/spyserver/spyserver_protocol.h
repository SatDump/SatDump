/*

SPY Server protocol structures and constants
Copyright (C) 2017 Youssef Touil youssef@live.com

*/


#pragma once

#include <stdint.h>
#include <limits.h>

#define SPYSERVER_PROTOCOL_VERSION (((2) << 24) | ((0) << 16) | (1700))

#define SPYSERVER_MAX_COMMAND_BODY_SIZE (256)
#define SPYSERVER_MAX_MESSAGE_BODY_SIZE (1 << 20)
#define SPYSERVER_MAX_DISPLAY_PIXELS (1 << 15)
#define SPYSERVER_MIN_DISPLAY_PIXELS (100)
#define SPYSERVER_MAX_FFT_DB_RANGE (150)
#define SPYSERVER_MIN_FFT_DB_RANGE (10)
#define SPYSERVER_MAX_FFT_DB_OFFSET (100)

enum DeviceType
{
  DEVICE_INVALID        = 0,
  DEVICE_AIRSPY_ONE     = 1,
  DEVICE_AIRSPY_HF      = 2,
  DEVICE_RTLSDR         = 3,
};

enum CommandType
{
  CMD_HELLO             = 0,
  CMD_GET_SETTING       = 1,
  CMD_SET_SETTING       = 2,
  CMD_PING              = 3,
};

enum SettingType
{
  SETTING_STREAMING_MODE        = 0,
  SETTING_STREAMING_ENABLED     = 1,
  SETTING_GAIN                  = 2,

  SETTING_IQ_FORMAT             = 100,
  SETTING_IQ_FREQUENCY          = 101,
  SETTING_IQ_DECIMATION         = 102,
  SETTING_IQ_DIGITAL_GAIN       = 103,

  SETTING_FFT_FORMAT            = 200,
  SETTING_FFT_FREQUENCY         = 201,
  SETTING_FFT_DECIMATION        = 202,
  SETTING_FFT_DB_OFFSET         = 203,
  SETTING_FFT_DB_RANGE          = 204,
  SETTING_FFT_DISPLAY_PIXELS    = 205,
};

enum StreamType
{
  STREAM_TYPE_STATUS    = 0,
  STREAM_TYPE_IQ        = 1,
  STREAM_TYPE_AF        = 2,
  STREAM_TYPE_FFT       = 4,
};


enum StreamingMode
{
  STREAM_MODE_IQ_ONLY       = STREAM_TYPE_IQ,
  STREAM_MODE_AF_ONLY       = STREAM_TYPE_AF,
  STREAM_MODE_FFT_ONLY      = STREAM_TYPE_FFT,
  STREAM_MODE_FFT_IQ        = STREAM_TYPE_FFT | STREAM_TYPE_IQ,
  STREAM_MODE_FFT_AF        = STREAM_TYPE_FFT | STREAM_TYPE_AF,
};

enum StreamFormat
{
  STREAM_FORMAT_INVALID     = 0,
  STREAM_FORMAT_UINT8       = 1,
  STREAM_FORMAT_INT16       = 2,
  STREAM_FORMAT_INT24       = 3,
  STREAM_FORMAT_FLOAT       = 4,
  STREAM_FORMAT_DINT4       = 5,
};

enum MessageType
{
  MSG_TYPE_DEVICE_INFO      = 0,
  MSG_TYPE_CLIENT_SYNC      = 1,
  MSG_TYPE_PONG             = 2,
  MSG_TYPE_READ_SETTING     = 3,

  MSG_TYPE_UINT8_IQ         = 100,
  MSG_TYPE_INT16_IQ         = 101,
  MSG_TYPE_INT24_IQ         = 102,
  MSG_TYPE_FLOAT_IQ         = 103,

  MSG_TYPE_UINT8_AF         = 200,
  MSG_TYPE_INT16_AF         = 201,
  MSG_TYPE_INT24_AF         = 202,
  MSG_TYPE_FLOAT_AF         = 203,

  MSG_TYPE_DINT4_FFT        = 300,
  MSG_TYPE_UINT8_FFT        = 301,
};

struct ClientHandshake
{
  uint32_t ProtocolVersion;
  uint32_t ClientNameLength;
};

struct CommandHeader
{
  uint32_t CommandType;
  uint32_t BodySize;
};

struct SettingTarget
{
  uint32_t StreamType;
  uint32_t SettingType;
};

struct MessageHeader
{
  uint32_t ProtocolID;
  uint32_t MessageType;
  uint32_t StreamType;
  uint32_t SequenceNumber;
  uint32_t BodySize;
};

struct DeviceInfo
{
  uint32_t DeviceType;
  uint32_t DeviceSerial;
  uint32_t MaximumSampleRate;
  uint32_t MaximumBandwidth;
  uint32_t DecimationStageCount;
  uint32_t GainStageCount;
  uint32_t MaximumGainIndex;
  uint32_t MinimumFrequency;
  uint32_t MaximumFrequency;
  uint32_t Resolution;
  uint32_t MinimumIQDecimation;
  uint32_t ForcedIQFormat;
};

struct ClientSync
{
  uint32_t CanControl;
  uint32_t Gain;
  uint32_t DeviceCenterFrequency;
  uint32_t IQCenterFrequency;
  uint32_t FFTCenterFrequency;
  uint32_t MinimumIQCenterFrequency;
  uint32_t MaximumIQCenterFrequency;
  uint32_t MinimumFFTCenterFrequency;
  uint32_t MaximumFFTCenterFrequency;
};

struct ComplexInt16
{
  int16_t real;
  int16_t imag;
};

struct ComplexUInt8
{
  uint8_t real;
  uint8_t imag;
};

enum ParserPhase {
  AcquiringHeader,
  ReadingData
};
