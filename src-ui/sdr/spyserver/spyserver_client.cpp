/*
 * Copyright 2018 Lucas Teske <lucas@teske.com.br>
 *   Based on Youssef Touil (youssef@live.com) C# implementation.
 *
 * Adapted from https://github.com/miweber67/spyserver_client
 */

#include "live.h"

#ifdef BUILD_LIVE

#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <memory>
#include <cstring>
#include <iomanip>
#include <fstream>

#include "spyserver_client.h"
#include "spyserver_protocol.h"

ss_client::ss_client(const std::string _ip,
                     const int _port,
                     const uint8_t _do_iq,
                     const uint8_t _do_fft,
                     const uint32_t _fft_points,
                     const uint8_t _samp_bits) : terminated(false),
                                                 streaming(false),
                                                 got_device_info(false),
                                                 receiver_thread(NULL),
                                                 header_data(new uint8_t[sizeof(MessageHeader)]),
                                                 body_buffer(NULL),
                                                 body_buffer_length(0),
                                                 parser_position(0),
                                                 last_sequence_number(0),
                                                 ip(_ip),
                                                 port(_port),

                                                 streaming_mode(STREAM_MODE_IQ_ONLY),
                                                 _fifo(NULL),
                                                 m_fifo_head(0),
                                                 m_fifo_tail(0),
                                                 m_fifo_size(10 * 1024 * 1024),
                                                 m_fft_count(0),
                                                 m_fft_period(100),
                                                 m_fft_bins(_fft_points),
                                                 m_iq_sample_rate(0),
                                                 _center_freq(0),
                                                 _gain(0),
                                                 _digitalGain(0),
                                                 m_do_iq(_do_iq),
                                                 m_do_fft(_do_fft),
                                                 m_sample_bits(_samp_bits)
{
#ifdef _WIN32
  WSAStartup();
#endif

  std::cerr << "SS_client(" << ip << ", " << port << ")" << std::endl;
  client = tcp_client(ip, port);

  streaming_mode = 0;
  if (m_do_iq)
  {
    streaming_mode |= STREAM_TYPE_IQ;
    _fifo = new uint8_t[m_fifo_size];
    if (!_fifo)
    {
      throw std::runtime_error(std::string(__FUNCTION__) + " " +
                               "Failed to allocate a sample FIFO!");
    }
  }
  if (m_do_fft)
  {
    // the spyserver I'm using won't send any fft data in fft_only mode.
    // have to use 'both' mode and just cut down the IQ as much as possible.
    streaming_mode |= STREAM_MODE_FFT_IQ;
    m_fft_bin_sums.clear();
    m_fft_bin_sums.resize(m_fft_bins, 0);
  }

  //   std::cerr << "Streaming mode is " << streaming_mode << std::endl;

  // Connect sends "HELLO" command to server
  // If connected also sends these commands in on_connect:
  // Currently sends only commands if mode is active, i.e., fft / iq
  // - streaming mode
  // - fft display pixels
  // - fft db offset
  // - fft db range
  // - iq digital gain
  // - fft format
  // - iq format
  connect();

  std::cerr << "SS_client: Ready" << std::endl;
}

// const std::string &ss_client::getName() {
//   switch (device_info.DeviceType) {
//   case DEVICE_INVALID:
//     return ss_client::NameNoDevice;
//   case DEVICE_AIRSPY_ONE:
//     return ss_client::NameAirspyOne;
//   case DEVICE_AIRSPY_HF:
//     return ss_client::NameAirspyHF;
//   case DEVICE_RTLSDR:
//     return ss_client::NameRTLSDR;
//   default:
//     return ss_client::NameUnknown;
//   }
// }

void ss_client::connect()
{
  bool hasError = false;
  if (receiver_thread != NULL)
  {
    return;
  }

  std::cerr << "SS_client: Trying to connect" << std::endl;
  client.connect_conn();
  is_connected = true;
  std::cerr << "SS_client: Connected" << std::endl;

  say_hello();
  cleanup();

  terminated = false;
  got_sync_info = false;
  got_device_info = false;

  std::exception error;

  receiver_thread = new std::thread(&ss_client::thread_loop, this);

  for (int i = 0; i < 1000 && !hasError; i++)
  {
    if (got_device_info)
    {
      if (device_info.DeviceType == DEVICE_INVALID)
      {
        error = std::runtime_error(std::string(__FUNCTION__) + " " + "Server is up but no device is available");
        hasError = true;
        break;
      }

      if (got_sync_info)
      {
        on_connect();
        return;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  disconnect();
  if (hasError)
  {
    throw error;
  }

  throw std::runtime_error(std::string(__FUNCTION__) + " " + "Server didn't send the device capability and synchronization info.");
}

void ss_client::disconnect()
{
  terminated = true;
  if (is_connected)
  {
    client.close_conn();
  }

  if (receiver_thread != NULL)
  {
    receiver_thread->join();
    receiver_thread = NULL;
  }

  cleanup();
}

void ss_client::on_connect()
{
  // Reference command order is:
  // - streaming mode
  // - fft display pixels
  // - fft db offset
  // - fft db range
  // - iq digital gain
  // - fft format
  // - iq format
  // --- these will have to wait until freq is set ---
  // - fft freq
  // - iq freq
  // - fft decim
  // - iq decim

  // It is unclear what should happen if a client wants only IQ or only FFT
  // so as a first approximation we will try setting only the relevent settings
  // to the mode(s) we are in.

  set_setting(SETTING_STREAMING_MODE, {streaming_mode});

  if (m_do_fft)
  {
    set_setting(SETTING_FFT_DISPLAY_PIXELS, {m_fft_bins});
    set_setting(SETTING_FFT_DB_OFFSET, {0x00});
    set_setting(SETTING_FFT_DB_RANGE, {0x7f});
  }

  // set_setting(SETTING_IQ_DIGITAL_GAIN, {0xFFFFFFFF}); //  sdrsharp sets this value to 0xffffffff
  set_setting(SETTING_IQ_DIGITAL_GAIN, {0x0});

  send_stream_format_commands();

  for (unsigned int i = device_info.MinimumIQDecimation; i <= device_info.DecimationStageCount; i++)
  {
    uint32_t sr = device_info.MaximumSampleRate / (1 << i);
    _sample_rates.push_back(std::pair<double, uint32_t>((double)sr, i));
  }

  std::sort(_sample_rates.begin(), _sample_rates.end());

  //  std::cerr << "SS_client: Supported Sample Rates: " << std::endl;
  //  for (std::pair<double, uint32_t> sr: _sample_rates) {
  //    std::cerr << "SS_client:   " << (unsigned int)sr.first << "\t\t" << sr.second << std::endl;
  //  }
}

bool ss_client::set_setting(uint32_t settingType, std::vector<uint32_t> params)
{
  std::vector<uint8_t> argBytes;
  if (params.size() > 0)
  {
    //    std::cerr << "set_setting: incoming params size: " << params.size() << " first val: " << params[0] << std::endl;
    argBytes = std::vector<uint8_t>(sizeof(SettingType) + params.size() * sizeof(uint32_t));
    uint8_t *settingBytes = (uint8_t *)&settingType;
    for (unsigned int i = 0; i < sizeof(uint32_t); i++)
    {
      argBytes[i] = settingBytes[i];
    }

    std::memcpy(&argBytes[0] + sizeof(uint32_t), &params[0], sizeof(uint32_t) * params.size());
  }
  else
  {
    argBytes = std::vector<uint8_t>();
  }

  return send_command(CMD_SET_SETTING, argBytes);
}

bool ss_client::say_hello()
{

  const uint8_t *protocolVersionBytes = (const uint8_t *)&ProtocolVersion;
  const uint8_t *softwareVersionBytes = (const uint8_t *)SoftwareID.c_str();
  std::vector<uint8_t> args = std::vector<uint8_t>(sizeof(ProtocolVersion) + SoftwareID.size());

  std::memcpy(&args[0], protocolVersionBytes, sizeof(ProtocolVersion));
  std::memcpy(&args[0] + sizeof(ProtocolVersion), softwareVersionBytes, SoftwareID.size());

  return send_command(CMD_HELLO, args);
}

void ss_client::cleanup()
{
  device_info.DeviceType = 0;
  device_info.DeviceSerial = 0;
  device_info.DecimationStageCount = 0;
  device_info.GainStageCount = 0;
  device_info.MaximumSampleRate = 0;
  device_info.MaximumBandwidth = 0;
  device_info.MaximumGainIndex = 0;
  device_info.MinimumFrequency = 0;
  device_info.MaximumFrequency = 0;

  _gain = 0;
  _digitalGain = 0;

  got_device_info = false;
  got_sync_info = false;

  last_sequence_number = ((uint32_t)-1);
  dropped_buffers = 0;
  down_stream_bytes = 0;

  parser_phase = AcquiringHeader;
  parser_position = 0;

  streaming = false;
  terminated = true;
}

void ss_client::thread_loop()
{
  parser_phase = AcquiringHeader;
  parser_position = 0;

  char buffer[BufferSize];
  try
  {
    while (!terminated)
    {
      if (terminated)
      {
        break;
      }
      uint32_t availableData = client.available_data();
      if (availableData > 0)
      {
        availableData = availableData > BufferSize ? BufferSize : availableData;
        client.receive_data(buffer, availableData);
        parse_message(buffer, availableData);
      }
      else if (m_do_iq)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      else if (m_do_fft)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }
  catch (std::exception &e)
  {
    std::cerr << "SS_client: Error in ThreadLoop: " << e.what() << std::endl;
  }
  if (body_buffer != NULL)
  {
    delete[] body_buffer;
    body_buffer = NULL;
  }

  cleanup();
}

void ss_client::parse_message(char *buffer, uint32_t len)
{
  down_stream_bytes++;

  int consumed;
  while (len > 0 && !terminated)
  {
    if (parser_phase == AcquiringHeader)
    {
      while (parser_phase == AcquiringHeader && len > 0)
      {
        consumed = parse_header(buffer, len);
        buffer += consumed;
        len -= consumed;
      }

      if (parser_phase == ReadingData)
      {
        //TODO: Do these really need to be checked every packet?!?!
        static uint8_t client_major = (SPYSERVER_PROTOCOL_VERSION >> 24) & 0xFF;
        static uint8_t client_minor = (SPYSERVER_PROTOCOL_VERSION >> 16) & 0xFF;

        uint8_t server_major = (header.ProtocolID >> 24) & 0xFF;
        uint8_t server_minor = (header.ProtocolID >> 16) & 0xFF;
        //uint16_t server_build = (header.ProtocolID & 0xFFFF);

        if (client_major != server_major || client_minor != server_minor)
        {
          throw std::runtime_error(std::string(__FUNCTION__) + " " + "Server is running an unsupported protocol version.");
        }

        if (header.BodySize > SPYSERVER_MAX_MESSAGE_BODY_SIZE)
        {
          throw std::runtime_error(std::string(__FUNCTION__) + " " + "The server is probably buggy.");
        }

        if (body_buffer == NULL || body_buffer_length < header.BodySize)
        {
          if (body_buffer != NULL)
          {
            delete[] body_buffer;
          }

          body_buffer = new uint8_t[header.BodySize];
        }
      }
    }

    if (parser_phase == ReadingData)
    {
      consumed = parse_body(buffer, len);
      buffer += consumed;
      len -= consumed;

      if (parser_phase == AcquiringHeader)
      {
        if (header.MessageType >= MSG_TYPE_UINT8_IQ && header.MessageType <= MSG_TYPE_FLOAT_IQ)
        {
          int32_t gap = header.SequenceNumber - last_sequence_number - 1;
          last_sequence_number = header.SequenceNumber;
          dropped_buffers += gap;
          if (gap > 0)
          {
            std::cerr << "SS_client: Lost " << gap << " frames from SpyServer!\n";
          }
        }
        handle_new_message();
      }
    }
  }
}

int ss_client::parse_header(char *buffer, uint32_t length)
{
  auto consumed = 0;

  while (length > 0)
  {
    int to_write = std::min<int>((uint32_t)(sizeof(MessageHeader) - parser_position), length);
    std::memcpy(&header + parser_position, buffer, to_write);
    /*    
    std::cerr << "Header:"
              << "\n   ProtocolID:     " << std::hex << header.ProtocolID
              << "\n   MessageType:     " << header.MessageType
              << "\n   StreamType:     " << std::dec << header.StreamType
              << "\n   SequenceNumber:     " << header.SequenceNumber
              << "\n   BodySize:     " << std::hex << header.BodySize
              << std::endl;
*/

    length -= to_write;
    buffer += to_write;
    parser_position += to_write;
    consumed += to_write;

    // limit message type
    header.MessageType = header.MessageType & 0xFFFF;

    if (parser_position == sizeof(MessageHeader))
    {
      parser_position = 0;
      if (header.BodySize > 0)
      {
        parser_phase = ReadingData;
      }

      return consumed;
    }
  }

  return consumed;
}

int ss_client::parse_body(char *buffer, uint32_t length)
{
  auto consumed = 0;

  while (length > 0)
  {
    int to_write = std::min<int>((int)header.BodySize - parser_position, length);
    std::memcpy(body_buffer + parser_position, buffer, to_write);
    length -= to_write;
    buffer += to_write;
    parser_position += to_write;
    consumed += to_write;

    if (parser_position == header.BodySize)
    {
      parser_position = 0;
      parser_phase = AcquiringHeader;
      return consumed;
    }
  }

  return consumed;
}

bool ss_client::send_command(uint32_t cmd, std::vector<uint8_t> args)
{
  if (!is_connected)
  {
    return false;
  }

  bool result;
  uint32_t headerLen = sizeof(CommandHeader);
  uint16_t argLen = args.size();
  uint8_t *buffer = new uint8_t[headerLen + argLen];

  CommandHeader header;
  header.CommandType = cmd;
  header.BodySize = argLen;

  for (uint32_t i = 0; i < sizeof(CommandHeader); i++)
  {
    buffer[i] = ((uint8_t *)(&header))[i];
  }

  if (argLen > 0)
  {
    for (uint16_t i = 0; i < argLen; i++)
    {
      buffer[i + headerLen] = args[i];
    }
  }

  try
  {
    //    std::cerr << "Sending command: " << cmd << " args: ";
    //    print_vec(args);
    //    std::cerr << std::endl;
    client.send_data((char *)buffer, headerLen + argLen);
    result = true;
  }
  catch (std::exception &e)
  {
    std::cerr << "caught exception while sending command.\n";
    result = false;
  }

  delete[] buffer;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return result;
}

void ss_client::handle_new_message()
{

  if (terminated)
  {
    return;
  }

  switch (header.MessageType)
  {
  case MSG_TYPE_DEVICE_INFO:
    process_device_info();
    break;
  case MSG_TYPE_CLIENT_SYNC:
    process_client_sync();
    break;
  case MSG_TYPE_UINT8_IQ:
    if (m_do_iq)
      process_uint8_samples();
    break;
  case MSG_TYPE_INT16_IQ:
    if (m_do_iq)
      process_int16_samples();
    break;
  case MSG_TYPE_FLOAT_IQ:
    if (m_do_iq)
      process_float_samples();
    break;
  case MSG_TYPE_UINT8_FFT:
    process_uint8_fft();
    break;
  default:
    std::cerr << "BAD MESSAGE TYPE: " << header.MessageType << "\n";
    break;
  }
}

void ss_client::process_device_info()
{
  std::memcpy(&device_info, body_buffer, sizeof(DeviceInfo));
  got_device_info = true;

  std::cerr << "\n**********\nDevice Info:"
            << "\n   Type:                 " << device_info.DeviceType
            << "\n   Serial:               " << device_info.DeviceSerial
            << "\n   MaximumSampleRate:    " << device_info.MaximumSampleRate
            << "\n   MaximumBandwidth:     " << device_info.MaximumBandwidth
            << "\n   DecimationStageCount: " << device_info.DecimationStageCount
            << "\n   GainStageCount:       " << device_info.GainStageCount
            << "\n   MaximumGainIndex:     " << device_info.MaximumGainIndex
            << "\n   MinimumFrequency:     " << device_info.MinimumFrequency
            << "\n   MaximumFrequency:     " << device_info.MaximumFrequency
            << "\n   Resolution:           " << device_info.Resolution
            << "\n   MinimumIQDecimation:  " << device_info.MinimumIQDecimation
            << "\n   ForcedIQFormat:       " << device_info.ForcedIQFormat
            << std::endl;
}

uint32_t ss_client::get_bandwidth()
{
  return device_info.MaximumBandwidth;
}

void ss_client::get_sampling_info(uint32_t &max_rate, uint32_t &decim_stages)
{
  if (got_device_info)
  {
    max_rate = device_info.MaximumSampleRate;
    decim_stages = device_info.DecimationStageCount;
  }
  else
  {
    max_rate = 0;
    decim_stages = 0;
  }
}

void ss_client::process_client_sync()
{

  std::memcpy(&m_cur_client_sync, body_buffer, sizeof(ClientSync));

  _gain = (double)m_cur_client_sync.Gain;
  _center_freq = (double)m_cur_client_sync.IQCenterFrequency;

  std::cerr << "\n**********\nClient sync:"
            << std::dec
            << "\n   Control:     " << (m_cur_client_sync.CanControl == 1 ? "Yes" : "No")
            << "\n   gain:        " << m_cur_client_sync.Gain
            << "\n   dev_ctr:     " << m_cur_client_sync.DeviceCenterFrequency
            << "\n   ch_ctr:      " << m_cur_client_sync.IQCenterFrequency
            << "\n   iq_ctr:      " << m_cur_client_sync.IQCenterFrequency
            << "\n   fft_ctr:     " << m_cur_client_sync.FFTCenterFrequency
            << "\n   min_iq_ctr:  " << m_cur_client_sync.MinimumIQCenterFrequency
            << "\n   max_iq_ctr:  " << m_cur_client_sync.MaximumIQCenterFrequency
            << "\n   min_fft_ctr: " << m_cur_client_sync.MinimumFFTCenterFrequency
            << "\n   max_fft_ctr: " << m_cur_client_sync.MaximumFFTCenterFrequency
            << std::endl;

  got_sync_info = true;
}

inline uint32_t ss_client::fifo_free(void)
{
  int32_t head = m_fifo_head;
  int32_t tail = m_fifo_tail;
  int32_t size = m_fifo_size;
  uint32_t avail;

  if (tail > head)
  {
    avail = tail - head;
  }
  else
  {
    avail = tail + (size - head);
  }

  return avail;
}

void ss_client::process_uint8_samples()
{

  _fifo_lock.lock();

  uint8_t *sample = (uint8_t *)body_buffer;

  //   std::cerr << "IN: tail\t" << m_fifo_tail << "\thead\t" << m_fifo_head << "\tfree\t" << fifo_free() << std::endl;

  // check for overflow
  if (fifo_free() < header.BodySize)
  {
    std::cerr << "O" << std::flush; // overflow notice
  }

  // byte fifo wrap
  // if fifo size is 10,
  // and head is 8 (next idx to write)
  // and tail is 4 (next idx to read)
  // then if 5 samples come in, we'll write 8, 9, 0, 1, 2 --> (8+5) % 10 = 3, new head
  // if head == tail+1, we are full; if head > tail, we have overwritten tail and
  // need to reset it to head

  if (m_fifo_head + header.BodySize < m_fifo_size)
  {
    // non-wrap case
    memcpy(&(_fifo[m_fifo_head]), sample, header.BodySize);
  }
  else
  {
    // wrap case
    uint64_t to_copy = m_fifo_size - m_fifo_head; // e.g. 10 - 8 = 2 bytes to copy, idx 8 and 9
                                                  //      uint64_t pval = (uint64_t)&(_fifo[0]);
                                                  //      std::cerr << "local buffer:    " << std::hex << pval << " - " << pval + m_fifo_size << std::endl;
                                                  //      pval = (uint64_t) sample;
                                                  //      std::cerr << "incoming buffer: " << std::hex << pval << " - " << pval + header.BodySize << std::endl;

    //      std::cerr << "part 1: copy 0x" <<  to_copy << " bytes from " << std::hex << pval << " - " << pval + to_copy << " to ";
    //      pval = (uint64_t)&(_fifo[m_fifo_head]);
    //      std::cerr << std::hex << pval << " - " << pval + to_copy << std::endl;
    memcpy(&(_fifo[m_fifo_head]), sample, to_copy);

    // remainder
    sample += to_copy; // advance by previous amount copied
    to_copy = header.BodySize - to_copy;
    //      pval = (uint64_t)sample;
    //      std::cerr << "part 2: copy 0x" << to_copy << " bytes from " << std::hex << pval << " - " << pval + to_copy << " to ";
    //      pval = (uint64_t)&(_fifo[0]);
    //      std::cerr << std::hex << pval << " - " << pval + to_copy << std::dec << std::endl;

    memcpy(&(_fifo[0]), sample, to_copy);
  }

  // in all cases, head is now here:
  m_fifo_head = (m_fifo_head + header.BodySize) % m_fifo_size;

  //   std::cerr << "    tail\t" << m_fifo_tail << "\thead\t" << m_fifo_head << "\tfree\t" << fifo_free() << std::endl;

  _fifo_lock.unlock();
  _samp_avail.notify_one();
}

void ss_client::process_int16_samples()
{

  _fifo_lock.lock();

  int16_t *sample = (int16_t *)body_buffer;

  /*
   std::cerr << "IN: "
             << header.BodySize << "B "
             << header.BodySize / 2 / 2 << " samps\ttail\t" 
             << m_fifo_tail << "\thead\t" 
             << m_fifo_head << "\tfree\t" 
             << fifo_free() << std::endl;
*/

  // check for overflow
  if (fifo_free() < header.BodySize)
  {
    std::cerr << "O" << std::flush; // overflow notice
  }

  // byte fifo wrap
  // if fifo size is 10,
  // and head is 8 (next idx to write)
  // and tail is 4 (next idx to read)
  // then if 5 samples come in, we'll write 8, 9, 0, 1, 2 --> (8+5) % 10 = 3, new head
  // if head == tail, we are full; if head > tail, we have overwritten tail and
  // need to reset it to head

  //   uint32_t copy_samps = header.BodySize / 2;
  if (m_fifo_head + header.BodySize < m_fifo_size)
  {
    // non-wrap case
    // memcpy works between server/client platforms of same endianness
    // RaspPi armv7, x86, ARM64, all little-endian. Good for now.
    memcpy(&(_fifo[m_fifo_head]), body_buffer, header.BodySize);
  }
  else
  {
    // wrap case
    int64_t to_copy = m_fifo_size - m_fifo_head;
    memcpy(&(_fifo[m_fifo_head]), sample, to_copy);

    uint8_t *src = (uint8_t *)sample;
    src += (m_fifo_size - m_fifo_head);
    to_copy = header.BodySize - (m_fifo_size - m_fifo_head);
    memcpy(&(_fifo[0]), src, to_copy);
  }

  // in all cases, head is now here:
  m_fifo_head = (m_fifo_head + header.BodySize) % m_fifo_size;

  //   std::cerr << "after in:    tail\t" << m_fifo_tail << "\thead\t" << m_fifo_head << "\tfree\t" << fifo_free() << std::endl;

  _fifo_lock.unlock();
  _samp_avail.notify_one();
}

void ss_client::process_float_samples()
{
}

void ss_client::set_stream_state()
{
  set_setting(SETTING_STREAMING_ENABLED, {(unsigned int)(streaming ? 1 : 0)});
}

bool ss_client::set_sample_rate_by_decim_stage(const uint32_t decim_stage)
{

  uint32_t requested_idx = 0xFFFFFFFF;

  if (decim_stage < device_info.DecimationStageCount)
  {
    for (unsigned int i = 0; i < _sample_rates.size(); i++)
    {
      if (_sample_rates[i].second == decim_stage)
      {
        requested_idx = i;
        break;
      }
    }
  }

  //   std::cerr << "Requested stage: " << decim_stage << "   --> index: " << requested_idx << std::endl;

  if (requested_idx >= _sample_rates.size())
  {
    std::cerr << "SS_client: Decimation stage not supported: " << decim_stage << std::endl;
    std::cerr << "SS_client: Supported Sample Rates: " << std::endl;
    for (std::pair<double, uint32_t> sr : _sample_rates)
    {
      std::cerr << "SS_client:   " << sr.first << "\t\t" << sr.second << std::endl;
    }
    std::stringstream ss;
    ss << "Unsupported decimation stage: " << decim_stage;
    throw std::runtime_error(ss.str());
  }

  return set_sample_rate_by_index(requested_idx);
}

bool ss_client::set_sample_rate_by_index(uint32_t requested_idx)
{

  if (requested_idx >= _sample_rates.size())
  {
    // This should not happen as this is an internal method and we should
    // have checked by now
    std::cerr << "SS_client: Internal error; requested_idx " << requested_idx
              << " out of bounds. Can only accept 0 to " << _sample_rates.size() - 1
              << std::endl;
    return false;
  }

  double sampleRate = _sample_rates[requested_idx].first;
  m_iq_sample_rate = sampleRate;

  if (m_do_fft)
  {
    m_fft_sample_rate = sampleRate;
    set_setting(SETTING_FFT_DECIMATION, {_sample_rates[requested_idx].second});
    //         std::cerr << "SS_client: Setting FFT sample rate to " << (unsigned int)m_fft_sample_rate << std::endl;
  }

  if (m_do_iq)
  {
    //         std::cerr << "SS_client: Setting IQ sample rate to " << sampleRate
    //            << " stage " << _sample_rates[requested_idx].second << std::endl;
    channel_decimation_stage_count = _sample_rates[requested_idx].second;
    set_setting(SETTING_IQ_DECIMATION, {channel_decimation_stage_count});

    // sdr# 'appears' to set the IQ format each time it sets the decimation
    // so we will try to do the same here.
    send_stream_format_commands();
  }
  else if (!m_do_iq)
  {
    // I am not sure I still believe this, so disabling for now.
    /*
         // For FFT-only, need full rate FFT but would like minimum rate IQ.
         // However, spyserver does not appear to provide high-rate fft and
         // low-rate IQ simultaneously. You must request as much IQ as you
         // want FFT data. :-(
         sampleRate = _sample_rates[_sample_rates.size() - 1].first;
         // set_setting(SETTING_IQ_DECIMATION, {_sample_rates[min_idx].second} );
         set_setting(SETTING_IQ_DECIMATION, {_sample_rates[requested_idx].second} );
         m_iq_sample_rate = sampleRate;
         std::cerr << "SS_client: Setting fft-only IQ sample rate to " << sampleRate << std::endl;
*/
  }

  return true;
}

bool ss_client::set_sample_rate(double sampleRate)
{

  uint32_t requested_idx = 0xFFFFFFFF;

  if (sampleRate <= 0xFFFFFFFF)
  {
    for (unsigned int i = 0; i < _sample_rates.size(); i++)
    {
      if (_sample_rates[i].first == sampleRate)
      {
        requested_idx = i;
      }
    }
  }

  if (requested_idx >= _sample_rates.size())
  {
    std::cerr << "SS_client: Sample rate not supported: " << sampleRate << std::endl;
    std::cerr << "SS_client: Supported Sample Rates: " << std::endl;
    for (std::pair<double, uint32_t> sr : _sample_rates)
    {
      std::cerr << "SS_client:   " << sr.first << std::endl;
    }
    std::stringstream ss("Unsupported samplerate: ");
    ss << sampleRate;
    throw std::runtime_error(ss.str());
  }

  return set_sample_rate_by_index(requested_idx);
}

bool ss_client::set_center_freq(double centerFrequency, size_t chan)
{
  // Order of operations for sdr sharp:
  // - fft freq
  // - iq freq
  // -- these will happen later --
  // - fft decim
  // - iq decim
  bool ret1 = true;
  bool ret2 = true;

  if (m_do_fft == 1)
  {
    ret1 = set_fft_center_freq(centerFrequency, chan);
    ret2 = set_iq_center_freq(centerFrequency, chan);
  }
  if (m_do_iq == 1)
  {
    ret2 = set_iq_center_freq(centerFrequency, chan);
  }
  return ret1 && ret2;
}

bool ss_client::set_iq_center_freq(double centerFrequency, size_t /*chan*/)
{

  if (centerFrequency < device_info.MinimumFrequency ||
      centerFrequency > device_info.MaximumFrequency)
  {
    std::cerr << "SS_client: Requested center IQ freq " << (centerFrequency / 1e6)
              << " outside device supported range: " << (unsigned int)device_info.MinimumFrequency
              << " - " << (unsigned int)device_info.MaximumFrequency << std::endl;
    return m_cur_client_sync.DeviceCenterFrequency;
  }

  // If we don't have control, requested center must be within bounds
  if (m_cur_client_sync.CanControl == 0)
  {
    if ((centerFrequency < m_cur_client_sync.MinimumIQCenterFrequency ||
         centerFrequency > m_cur_client_sync.MaximumIQCenterFrequency))
    {
      std::cerr << "SS_client: Requested center IQ freq " << (centerFrequency / 1e6)
                << " outside currently allowed range: " << m_cur_client_sync.MinimumIQCenterFrequency
                << " - " << m_cur_client_sync.MaximumIQCenterFrequency << std::endl;
      return m_cur_client_sync.IQCenterFrequency;
    }
  }

  // If we do have control or the request is in bounds, OK to set
  std::vector<uint32_t> param(1);
  param[0] = centerFrequency;
  //   std::cerr << "SS_client: Setting SETTING_IQ_FREQUENCY" << std::endl;
  set_setting(SETTING_IQ_FREQUENCY, param);
  // get updated client sync block
  send_stream_format_commands();

  // can't know if this actually succeeded until updated client sync block arrives
  return true;
}

bool ss_client::set_fft_center_freq(double centerFrequency, size_t /*chan*/)
{

  if (centerFrequency < device_info.MinimumFrequency ||
      centerFrequency > device_info.MaximumFrequency)
  {
    std::cerr << "SS_client: Requested center FFT freq " << (centerFrequency / 1e6)
              << " outside device supported range: " << (unsigned int)device_info.MinimumFrequency
              << " - " << (unsigned int)device_info.MaximumFrequency << std::endl;
    return false;
  }

  // If we don't have control, requested center must be within bounds
  if (m_cur_client_sync.CanControl == 0)
  {
    if ((centerFrequency < m_cur_client_sync.MinimumFFTCenterFrequency ||
         centerFrequency > m_cur_client_sync.MaximumFFTCenterFrequency))
    {
      std::cerr << "SS_client: Requested center FFT freq " << (centerFrequency / 1e6)
                << " outside currently allowed range: " << m_cur_client_sync.MinimumFFTCenterFrequency
                << " - " << m_cur_client_sync.MaximumFFTCenterFrequency << std::endl;
      return false;
    }
  }

  // If we do have control or the request is in bounds, OK to set
  std::vector<uint32_t> param(1);
  param[0] = centerFrequency;
  //   std::cerr << "SS_client: Setting SETTING_FFT_FREQUENCY" << std::endl;
  set_setting(SETTING_FFT_FREQUENCY, param);
  // get updated client sync block
  send_stream_format_commands();

  // can't know if this actually succeeded until updated client sync block arrives
  return true;
}

void ss_client::process_uint8_fft()
{

  size_t num_pts = header.BodySize;
  uint8_t *val = (uint8_t *)body_buffer;

  //   std::cerr << "Got " << num_pts << " FFT points\n";

  m_fft_data_lock.lock();
  for (size_t i = 0; i < num_pts; ++i)
  {
    m_fft_bin_sums[i] += *val;
    ++val;
  }
  ++m_fft_count;
  m_fft_data_lock.unlock();
  m_fft_avail.notify_one();
}

void ss_client::get_fft_data(std::vector<uint32_t> &outdata, int &outperiods)
{

  std::unique_lock<std::mutex> lock(m_fft_data_lock);

  while (0 == m_fft_count)
  {
    m_fft_avail.wait(lock);
  }

  outdata = m_fft_bin_sums;
  outperiods = m_fft_count;

  m_fft_bin_sums.clear();
  m_fft_bin_sums.resize(m_fft_bins);
  m_fft_count = 0;

  // unique lock automatically unlocks
}

ss_client::~ss_client()
{
  disconnect();
  if (_fifo)
  {
    delete _fifo;
    _fifo = NULL;
  }
  delete[] header_data;
  header_data = NULL;
}

bool ss_client::start()
{
  if (!streaming)
  {
    std::cerr << "SS_client: Starting Streaming" << std::endl;
    streaming = true;
    down_stream_bytes = 0;
    set_stream_state();
    return true;
  }
  return false;
}

bool ss_client::stop()
{
  if (streaming)
  {
    std::cerr << "SS_client: Stopping Streaming" << std::endl;
    streaming = false;
    down_stream_bytes = 0;
    set_stream_state();
    return true;
  }
  return false;
}

template <class T>
int ss_client::get_iq_data(const int batch_size,
                           T *out_array)
{

  if (!streaming || !m_do_iq)
  {
    return 0;
  }
  //   std::cerr << "ss_client::get_iq_data: sizeof(T)=" << sizeof(T) << std::endl;
  std::unique_lock<std::mutex> lock(_fifo_lock);

  // if size is 10, head is 3, and tail is 5, bytes
  // 5, 6, 7, 8, 9, 0, 1, 2 are available (8 bytes)
  // 3 - 5 = -2 + 10 = 8
  // this doesn't work if size > int32_t max
  // each sample counts as I + Q, so two values, each sizeof(T) bytes long
  int32_t samps_avail = ((m_fifo_size - fifo_free()) / 2) / sizeof(T);

  // don't peg the cpu checking, but don't wait too long beteween checks.
  // max_wait is how long it should take to receive a complete batch.
  unsigned int max_wait = (((double)batch_size / m_iq_sample_rate) * 1000) / 3;
  //   std::cerr << "batch size: " << batch_size << "   iq_samp_rate: " << m_iq_sample_rate << std::endl;
  //   std::cerr << "max_wait for iq data check is " << max_wait << "ms\n";
  while (samps_avail < batch_size)
  {
    //      std::cerr << "   Waiting with " << samps_avail << " samples available, want " << batch_size << std::endl;
    _samp_avail.wait(lock);
    samps_avail = ((m_fifo_size - fifo_free()) / 2) / sizeof(T);
    std::this_thread::sleep_for(std::chrono::milliseconds(max_wait));
  }

  //   std::cerr << "   Releasing with " << samps_avail << " samples available, want " << batch_size << std::endl;
  //   std::cerr << "OUT: tail\t" << m_fifo_tail << "\thead\t" << m_fifo_head << "\tfree\t" << fifo_free() << std::endl;

  // if size is 10, tail is 5, and batch is 5, OK
  // 5 + 5 = 10 (read 5, 6, 7, 8, 9 and reset tail)
  // but if batch is 6, wrap
  // 5 + 6 = 11
  // in both cases new tail is tail + batch_size % size

  // units below are all bytes except for batch_size, which is 'samples'
  uint32_t batch_bytes = (batch_size * sizeof(T)) * 2;
  if (m_fifo_tail + batch_bytes <= m_fifo_size)
  {
    // non-wrap case
    memcpy(out_array, &(_fifo[m_fifo_tail]), batch_bytes);
  }
  else
  {
    uint8_t *dst = (uint8_t *)out_array;
    memcpy(dst, &(_fifo[m_fifo_tail]), m_fifo_size - m_fifo_tail);
    memcpy(dst + (m_fifo_size - m_fifo_tail),
           &(_fifo[0]),
           batch_bytes - (m_fifo_size - m_fifo_tail));
  }

  // move tail
  m_fifo_tail = (m_fifo_tail + batch_bytes) % m_fifo_size;

  return (batch_bytes / sizeof(T)) / 2;
}

double ss_client::get_sample_rate()
{
  if (m_do_iq)
    return m_iq_sample_rate;
  if (m_do_fft)
    return m_fft_sample_rate;
  return 0;
}

double ss_client::get_center_freq(size_t /*chan*/)
{
  return _center_freq;
}

std::vector<std::string> ss_client::get_gain_names(size_t /*chan*/)
{
  std::vector<std::string> names;

  if (m_cur_client_sync.CanControl)
  {
    names.push_back("LNA");
  }
  names.push_back("Digital");

  return names;
}

bool ss_client::set_gain_mode(bool /*automatic*/, size_t chan)
{
  return get_gain_mode(chan);
}

bool ss_client::get_gain_mode(size_t /*chan*/)
{
  return false;
}

double ss_client::set_gain(double gain, size_t /*chan*/)
{
  if (m_cur_client_sync.CanControl)
  {
    _gain = gain;
    set_setting(SETTING_GAIN, {(uint32_t)gain});
  }
  else
  {
    std::cerr << "SS_client: The server does not allow you to change the gains." << std::endl;
  }

  return _gain;
}

double ss_client::set_gain(double gain, const std::string &name, size_t chan)
{
  if (name == "Digital")
  {
    _digitalGain = gain;
    std::cerr << "SS_client: Setting digital gain not currently supported." << std::endl;
    //    set_setting(SETTING_IQ_DIGITAL_GAIN, {((uint32_t)gain) * 0xFFFFFFFF});
    //    set_setting(SETTING_IQ_DIGITAL_GAIN, {(unsigned int)gain * 0xFFFFFFFF});
    return _gain;
  }

  return set_gain(gain, chan);
}

double ss_client::get_gain(size_t chan)
{
  return chan == 0 ? _gain : _digitalGain;
}

double ss_client::get_gain(const std::string &name, size_t chan)
{
  if (name == "Digital")
  {
    return _digitalGain;
  }

  return get_gain(chan);
}

void ss_client::send_stream_format_commands()
{
  if (m_do_fft)
  {
    //      std::cerr << "SS_client: Sending fft format command" << std::endl;
    set_setting(SETTING_FFT_FORMAT, {STREAM_FORMAT_UINT8});
  }
  //   if( m_do_iq ) {
  if (1)
  {
    //      std::cerr << "SS_client: Sending iq format command" << std::endl;
    if (m_sample_bits == 16)
    {
      set_setting(SETTING_IQ_FORMAT, {STREAM_FORMAT_INT16});
    }
    else
    {
      set_setting(SETTING_IQ_FORMAT, {STREAM_FORMAT_UINT8});
    }
  }
}

template int ss_client::get_iq_data<int16_t>(const int batch_size, int16_t *out_array);
template int ss_client::get_iq_data<uint8_t>(const int batch_size, uint8_t *out_array);

#endif