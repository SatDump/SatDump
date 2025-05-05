#include <iostream>
#include "librfnm_api.h"
#include <queue>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <math.h>
#include <array>


#pragma once

#if defined(__GNUC__)
#define RFNM_PACKED_STRUCT( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#define MSDLL
#elif defined(_MSC_VER)
#define RFNM_PACKED_STRUCT( __Declaration__ ) __pragma(pack(push,1)) __Declaration__ __pragma(pack(pop))
#define MSDLL __declspec(dllexport) 
#endif

enum librfnm_transport {
    LIBRFNM_TRANSPORT_LOCAL,
    LIBRFNM_TRANSPORT_USB,
    LIBRFNM_TRANSPORT_ETH,
    LIBRFNM_TRANSPORT_FIND
};

enum librfnm_req_type {
    LIBRFNM_REQ_ALL = 0xff,
    LIBRFNM_REQ_TX = (0x1 << 1),
    LIBRFNM_REQ_RX = (0x1 << 2),
    LIBRFNM_REQ_HWINFO = (0x1 << 3),
    LIBRFNM_REQ_DEV_STATUS = (0x1 << 4),
};

enum librfnm_debug_level {
    LIBRFNM_DEBUG_NONE = 0,
    LIBRFNM_DEBUG_ERROR = (0x1 << 1),
    LIBRFNM_DEBUG_INFO = (0x1 << 2),
    LIBRFNM_DEBUG_VERBOSE = (0x1 << 3),
};

enum librfnm_stream_format {
    LIBRFNM_STREAM_FORMAT_CS8 = 2,
    LIBRFNM_STREAM_FORMAT_CS16 = 4,
    LIBRFNM_STREAM_FORMAT_CF32 = 8,
};

enum librfnm_tx_latency_policy {
    // average number of tx buffers in the pipeline times two
    LIBRFNM_TX_LATENCY_POLICY_DEFAULT,
    // times 1.5
    LIBRFNM_TX_LATENCY_POLICY_AGGRESSIVE,
    // times 4
    LIBRFNM_TX_LATENCY_POLICY_RELAXED,
};

/*enum librfnm_apply_ch {
    LIBRFNM_APPLY_CH0 = (0x1 << 0),
    LIBRFNM_APPLY_CH1 = (0x1 << 1),
    LIBRFNM_APPLY_CH2 = (0x1 << 2),
    LIBRFNM_APPLY_CH3 = (0x1 << 3),
    LIBRFNM_APPLY_CH4 = (0x1 << 4),
    LIBRFNM_APPLY_CH5 = (0x1 << 5),
    LIBRFNM_APPLY_CH6 = (0x1 << 6),
    LIBRFNM_APPLY_CH7 = (0x1 << 7),
};*/

#define RFNM_USB_VID (0x15A2)
#define RFNM_USB_PID (0x8C)
#define RFNM_USB_PID_BOOST (0x8D)

#define RFNM_B_REQUEST (100)


#define LIBRFNM_THREAD_COUNT 16

#define LIBRFNM_MIN_RX_BUFCNT 500
#define LIBRFNM_RX_RECOMB_BUF_LEN (300)


#define LIBRFNM_CH0 (0x1 << 0)
#define LIBRFNM_CH1 (0x1 << 1)
#define LIBRFNM_CH2 (0x1 << 2)
#define LIBRFNM_CH3 (0x1 << 3)
#define LIBRFNM_CH4 (0x1 << 4)
#define LIBRFNM_CH5 (0x1 << 5)
#define LIBRFNM_CH6 (0x1 << 6)
#define LIBRFNM_CH7 (0x1 << 7)

#define LIBRFNM_APPLY_CH0_TX (0x1 << 0)
#define LIBRFNM_APPLY_CH1_TX (0x1 << 1)
#define LIBRFNM_APPLY_CH2_TX (0x1 << 2)
#define LIBRFNM_APPLY_CH3_TX (0x1 << 3)
#define LIBRFNM_APPLY_CH4_TX (0x1 << 4)
#define LIBRFNM_APPLY_CH5_TX (0x1 << 5)
#define LIBRFNM_APPLY_CH6_TX (0x1 << 6)
#define LIBRFNM_APPLY_CH7_TX (0x1 << 7)

#define LIBRFNM_APPLY_CH0_RX ((0x1 << 0) << 8)
#define LIBRFNM_APPLY_CH1_RX ((0x1 << 1) << 8)
#define LIBRFNM_APPLY_CH2_RX ((0x1 << 2) << 8)
#define LIBRFNM_APPLY_CH3_RX ((0x1 << 3) << 8)
#define LIBRFNM_APPLY_CH4_RX ((0x1 << 4) << 8)
#define LIBRFNM_APPLY_CH5_RX ((0x1 << 5) << 8)
#define LIBRFNM_APPLY_CH6_RX ((0x1 << 6) << 8)
#define LIBRFNM_APPLY_CH7_RX ((0x1 << 7) << 8)


#define RFNM_MHZ_TO_HZ(MHz) (MHz * 1000 * 1000ul)
#define RFNM_HZ_TO_MHZ(Hz) (Hz / (1000ul * 1000ul))
#define RFNM_HZ_TO_KHZ(Hz) (Hz / 1000ul)





struct librfnm_transport_status {
    enum librfnm_transport transport;
    int usb_boost_connected;
    int theoretical_mbps;
    enum librfnm_stream_format rx_stream_format;
    enum librfnm_stream_format tx_stream_format;
    int boost_pp_tx;
    int boost_pp_rx;
};

//RFNM_PACKED_STRUCT(
    struct librfnm_status {
        struct librfnm_transport_status transport_status;

        struct rfnm_dev_hwinfo hwinfo;
        struct rfnm_dev_tx_ch_list tx;
        struct rfnm_dev_rx_ch_list rx;

        struct rfnm_dev_status dev_status;
        
        std::chrono::time_point<std::chrono::high_resolution_clock> last_dev_time;
        //std::mutex dev_status_mutex;
    };
//);


RFNM_PACKED_STRUCT(
    struct librfnm_rx_buf {
        uint8_t* buf;
        uint32_t phytimer;
        uint32_t adc_cc;
        uint64_t usb_cc;
        uint32_t adc_id;
        //bool operator<(const librfnm_rx_buf * lrb) const {
        //    return usb_cc < lrb->usb_cc;
        //}
    }
);

RFNM_PACKED_STRUCT(
    struct librfnm_tx_buf {
    uint8_t* buf;
    uint32_t phytimer;
    uint32_t dac_cc;
    uint64_t usb_cc;
    uint32_t dac_id;
}
);



class librfnm_rx_buf_compare {
public:
    bool operator()(struct librfnm_rx_buf* lra, struct librfnm_rx_buf* lrb) {
        return (lra->usb_cc) > (lrb->usb_cc);
    }
};

struct librfnm_rx_buf_s {
    std::queue<struct librfnm_rx_buf*> in;
    std::priority_queue<struct librfnm_rx_buf*, std::vector<struct librfnm_rx_buf*>, librfnm_rx_buf_compare> out[4];
    std::mutex in_mutex;
    std::mutex out_mutex;
    std::condition_variable cv;
    uint8_t required_adc_id;
    uint64_t usb_cc[4];
    uint64_t qbuf_cnt;


    uint64_t usb_cc_benchmark[4];
    std::mutex benchmark_mutex;
    uint8_t last_benchmark_adc;
};

struct librfnm_tx_buf_s {
    std::queue<struct librfnm_tx_buf*> in;
    std::queue<struct librfnm_tx_buf*> out;
    std::mutex in_mutex;
    std::mutex out_mutex;
    //std::mutex cc_mutex;
    std::condition_variable cv;
    uint64_t usb_cc;
    uint64_t qbuf_cnt;
};

struct librfnm_thread_data_s {
    int ep_id;
    int tx_active;
    int rx_active;
    int shutdown_req;
    std::condition_variable cv;
    std::mutex cv_mutex;
};


struct _librfnm_usb_handle;



class librfnm {
public:
    MSDLL explicit librfnm(enum librfnm_transport transport, std::string address = "", enum librfnm_debug_level dbg = LIBRFNM_DEBUG_NONE);
    MSDLL ~librfnm();
public:

    MSDLL static std::vector<struct rfnm_dev_hwinfo> find(enum librfnm_transport transport, std::string address = "", int bind = 0);

    MSDLL rfnm_api_failcode get(enum librfnm_req_type type);

    MSDLL rfnm_api_failcode set(uint16_t applies, bool confirm_execution = true, uint32_t wait_for_ms = 1000);

    MSDLL rfnm_api_failcode rx_stream(enum librfnm_stream_format format, int* bufsize);
    
    MSDLL rfnm_api_failcode rx_qbuf(struct librfnm_rx_buf* buf);

    MSDLL rfnm_api_failcode rx_dqbuf(struct librfnm_rx_buf** buf, uint8_t ch_ids = 0, uint32_t wait_for_ms = 20);

    MSDLL rfnm_api_failcode tx_stream(enum librfnm_stream_format format, int* bufsize, enum librfnm_tx_latency_policy policy = LIBRFNM_TX_LATENCY_POLICY_DEFAULT);

    MSDLL rfnm_api_failcode tx_qbuf(struct librfnm_tx_buf* buf, uint32_t wait_for_ms = 20);

    MSDLL rfnm_api_failcode tx_dqbuf(struct librfnm_tx_buf** buf);

    MSDLL static enum rfnm_rf_path string_to_rf_path(std::string path);

    MSDLL static std::string rf_path_to_string(enum rfnm_rf_path path);



    //static int format_to_bytes_per_ele(enum librfnm_stream_format format);


    struct librfnm_status* s;

private:
    void threadfn(size_t thread_index);

private:
    


    _librfnm_usb_handle *usb_handle;
    //libusb_device_handle* usb_handle->primary{};
    //libusb_device_handle* usb_handle->boost{};



    std::mutex librfnm_s_dev_status_mutex;

    std::mutex librfnm_s_transport_pp_mutex;




    struct librfnm_rx_buf_s librfnm_rx_s;
    struct librfnm_tx_buf_s librfnm_tx_s;
    struct librfnm_thread_data_s librfnm_thread_data[LIBRFNM_THREAD_COUNT];

    std::array<std::thread, LIBRFNM_THREAD_COUNT> librfnm_thread_c{};


    MSDLL bool unpack_12_to_cs16(uint8_t* dest, uint8_t* src, size_t sample_cnt);
    MSDLL bool unpack_12_to_cf32(uint8_t* dest, uint8_t* src, size_t sample_cnt);
    MSDLL bool unpack_12_to_cs8(uint8_t* dest, uint8_t* src, size_t sample_cnt);
    MSDLL void pack_cs16_to_12(uint8_t* dest, uint8_t* src8, int sample_cnt);


    MSDLL int single_ch_id_bitmap_to_adc_id(uint8_t ch_ids);
    MSDLL void dqbuf_overwrite_cc(uint8_t adc_id, int acquire_lock);
    MSDLL int dqbuf_is_cc_continuous(uint8_t adc_id, int acquire_lock);

};