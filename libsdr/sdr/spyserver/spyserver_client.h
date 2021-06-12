/*
 * Inspired by the gnuradio spyserver code, but significantly altered.
 *
 * Original source: 
 *   https://github.com/racerxdl/gr-osmosdr/tree/master/lib/spyserver
 * 2019 Mike Weber <github@likesgadgets.com>
 * 
 * Original Copyright 2013 Dimitri Stolnikov <horiz0n@gmx.net>
 *
 */
#ifndef SS_CLIENT_H
#define SS_CLIENT_H

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <complex>
#include <iomanip>
#include <deque>

#include "spyserver_protocol.h"
#include "tcp_client.h"

class ss_client
{
public:
   ss_client(const std::string _ip,
                const int _port,
                const uint8_t _do_iq,
                const uint8_t _do_fft,
                const uint32_t _fft_points,
                const uint8_t _sample_bits);

   ~ss_client();

   bool start();
   bool stop();

   template <class T>
   int get_iq_data(const int batch_size, T *output_items);

   void get_fft_data(std::vector<uint32_t> &outdata, int &outperiods);
   void get_sampling_info(uint32_t &max_rate, uint32_t &decim_stages);

   bool set_sample_rate(double rate);
   bool set_sample_rate_by_decim_stage(const uint32_t decim_stage);
   double get_sample_rate(void);

   bool set_center_freq(double freq, size_t chan = 0);
   bool set_iq_center_freq(double freq, size_t chan = 0);
   bool set_fft_center_freq(double freq, size_t chan = 0);
   // return the bandwidth used for fft range
   uint32_t get_bandwidth();
   // which would this return? TODO: fix
   double get_center_freq(size_t chan = 0);

   std::vector<std::string> get_gain_names(size_t chan = 0);
   bool set_gain_mode(bool automatic, size_t chan = 0);
   bool get_gain_mode(size_t chan = 0);
   double set_gain(double gain, size_t chan = 0);
   double set_gain(double gain, const std::string &name, size_t chan = 0);
   double get_gain(size_t chan = 0);
   double get_gain(const std::string &name, size_t chan = 0);

private:
   static constexpr unsigned int BufferSize = 64 * 1024;
   const uint32_t ProtocolVersion = SPYSERVER_PROTOCOL_VERSION;
   const std::string SoftwareID = std::string("gr-osmosdr");
   const std::string NameNoDevice = std::string("SpyServer - No Device");
   const std::string NameAirspyOne = std::string("SpyServer - Airspy One");
   const std::string NameAirspyHF = std::string("SpyServer - Airspy HF+");
   const std::string NameRTLSDR = std::string("SpyServer - RTLSDR");
   const std::string NameUnknown = std::string("SpyServer - Unknown Device");

   uint32_t channel_decimation_stage_count;
   tcp_client client;

   void connect();
   void disconnect();
   void thread_loop();
   bool say_hello();
   void cleanup();
   void on_connect();

   bool set_setting(uint32_t settingType, std::vector<uint32_t> params);
   bool send_command(uint32_t cmd, std::vector<uint8_t> args);
   void parse_message(char *buffer, uint32_t len);
   int parse_header(char *buffer, uint32_t len);
   int parse_body(char *buffer, uint32_t len);
   void process_device_info();
   void process_client_sync();
   void process_uint8_samples();
   void process_int16_samples();
   void process_float_samples();
   void process_uint8_fft();
   void handle_new_message();
   void set_stream_state();
   bool set_sample_rate_by_index(uint32_t requested_idx);
   void send_stream_format_commands();

   uint32_t fifo_free(void);

   std::atomic_bool terminated;
   std::atomic_bool streaming;
   std::atomic_bool got_device_info;
   std::atomic_bool got_sync_info;
   std::atomic_bool is_connected;
   std::thread *receiver_thread;

   uint32_t dropped_buffers;
   std::atomic<int64_t> down_stream_bytes;

   uint8_t *header_data;
   uint8_t *body_buffer;
   uint64_t body_buffer_length;
   uint32_t parser_position;
   uint32_t last_sequence_number;

   std::string ip;
   int port;

   DeviceInfo device_info;
   ClientSync m_cur_client_sync;
   MessageHeader header;

   uint32_t streaming_mode;
   uint32_t parser_phase;

   //   std::deque<_Tsz>* _fifo;
   uint8_t *_fifo;
   uint32_t m_fifo_head;
   uint32_t m_fifo_tail;
   const uint32_t m_fifo_size;

   std::vector<uint32_t> m_fft_bin_sums;
   uint32_t m_fft_count;
   uint32_t m_fft_period; // the number of ffts to be averaged and reported
   uint32_t m_fft_bins;
   std::condition_variable m_fft_avail;

   std::mutex _fifo_lock;
   std::mutex m_fft_data_lock;
   std::condition_variable _samp_avail;

   std::vector<std::pair<double, uint32_t>> _sample_rates;
   double m_iq_sample_rate;
   double m_fft_sample_rate;
   double _center_freq;
   double _gain;
   double _digitalGain;
   uint8_t m_do_iq;
   uint8_t m_do_fft;
   uint8_t m_sample_bits;
};

#endif /* SS_CLIENT_H */
