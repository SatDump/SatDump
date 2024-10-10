#include "librfnm/librfnm.h"
//#include <spdlog/spdlog.h>
#include <libusb-1.0/libusb.h>
#include <cstring>
#include <algorithm>

struct _librfnm_usb_handle {
    libusb_device_handle* primary{};
    libusb_device_handle* boost{};
};


MSDLL bool librfnm::unpack_12_to_cs16(uint8_t* dest, uint8_t* src, size_t sample_cnt) {
    uint64_t buf{};
    uint64_t r0{};
    uint64_t* dest_64{};
    uint64_t* src_64{};
    //uint32_t input_bytes_cnt;
    //input_bytes_cnt = sample_cnt * 3;

    if (sample_cnt % 2) {
        printf("RFNM::Conversion::unpack12to16() -> sample_cnt %d is not divisible by 2\n", sample_cnt);
        return false;
    }

    // process two samples at a time
    sample_cnt = sample_cnt / 2;
    for (size_t c = 0; c < sample_cnt; c++) {
        src_64 = (uint64_t*)((uint8_t*)src + (c * 6));
        buf = *(src_64); //unaligned read?
        r0 = 0;
        r0 |= (buf & (0xfffll << 0)) << 4;
        r0 |= (buf & (0xfffll << 12)) << 8;
        r0 |= (buf & (0xfffll << 24)) << 12;
        r0 |= (buf & (0xfffll << 36)) << 16;

        dest_64 = (uint64_t*)(dest + (c * 8));
        *dest_64 = r0;
    }
    return true;
}


MSDLL bool librfnm::unpack_12_to_cf32(uint8_t* dest, uint8_t* src, size_t sample_cnt) {
    uint64_t buf{};
    uint64_t r0{};
    uint64_t* dest_64{};
    uint64_t* src_64{};
    //uint32_t input_bytes_cnt;
    //input_bytes_cnt = sample_cnt * 3;

    if (sample_cnt % 2) {
        printf("RFNM::Conversion::unpack12to16() -> sample_cnt %d is not divisible by 2\n", sample_cnt);
        return false;
    }

    // process two samples at a time
    sample_cnt = sample_cnt / 2;
    for (size_t c = 0; c < sample_cnt; c++) {
        src_64 = (uint64_t*)((uint8_t*)src + (c * 6));
        buf = *(src_64); //unaligned read?

        float* i1, * i2, * q1, * q2;

        i1 = (float*)((uint8_t*)dest + (c * 16) + 0);
        q1 = (float*)((uint8_t*)dest + (c * 16) + 4);
        i2 = (float*)((uint8_t*)dest + (c * 16) + 8);
        q2 = (float*)((uint8_t*)dest + (c * 16) + 12);


        *i1 = ((int16_t)((buf & (0xfffll << 0)) << 4)) / 32767.0f;
        *q1 = ((int16_t)((buf & (0xfffll << 12)) >> 8)) / 32767.0f;
        *i2 = ((int16_t)((buf & (0xfffll << 24)) >> 20)) / 32767.0f;
        *q2 = ((int16_t)((buf & (0xfffll << 36)) >> 32)) / 32767.0f;
    }
    return true;
}


MSDLL bool librfnm::unpack_12_to_cs8(uint8_t* dest, uint8_t* src, size_t sample_cnt) {
    uint64_t buf{};
    uint32_t r0{};
    uint32_t* dest_32{};
    uint64_t* src_64{};
    //uint32_t input_bytes_cnt;
    //input_bytes_cnt = sample_cnt * 3;

    if (sample_cnt % 2) {
        printf("RFNM::Conversion::unpack12to16() -> sample_cnt %d is not divisible by 2\n", sample_cnt);
        return false;
    }

    // process two samples at a time
    sample_cnt = sample_cnt / 2;
    for (size_t c = 0; c < sample_cnt; c++) {
        src_64 = (uint64_t*)((uint8_t*)src + (c * 6));
        buf = *(src_64);
        r0 = 0;
        r0 |= (buf & (0xffll << 4)) >> 4;
        r0 |= (buf & (0xffll << 16)) >> 8;
        r0 |= (buf & (0xffll << 28)) >> 12;
        r0 |= (buf & (0xffll << 40)) >> 16;

        dest_32 = (uint32_t*)((uint8_t*)dest + (c * 4));
        *dest_32 = r0;
    }
    return true;
}



MSDLL void librfnm::pack_cs16_to_12(uint8_t* dest, uint8_t* src8, int sample_cnt) {
    uint64_t buf;
    uint64_t r0;
    int32_t c;
    uint64_t* dest_64;
    uint64_t* src;

    src = (uint64_t*)src8;

    sample_cnt = sample_cnt / 2;

    //printk("%p %p\n", dest, src);


    for (c = 0; c < sample_cnt; c++) {
        buf = *(src + c);
        r0 = 0;
        r0 |= (buf & (0xfffll << 4)) >> 4;
        r0 |= (buf & (0xfffll << 20)) >> 8;
        r0 |= (buf & (0xfffll << 36)) >> 12;
        r0 |= (buf & (0xfffll << 52)) >> 16;

        //printk("set %llux to %p (c=%d)\n", r0,  ( void * ) (dest + (c * 3)), c);
        dest_64 = (uint64_t*)(dest + (c * 6));
        *dest_64 = r0;

        //if(c > 10)
        //	return;
    }

}

void librfnm::threadfn(size_t thread_index) {
    struct rfnm_rx_usb_buf* lrxbuf = (struct rfnm_rx_usb_buf*)malloc(RFNM_USB_RX_PACKET_SIZE);
    struct rfnm_tx_usb_buf* ltxbuf = (struct rfnm_tx_usb_buf*)malloc(RFNM_USB_TX_PACKET_SIZE);
    int transferred;

    auto& tpm = librfnm_thread_data[thread_index];

    int r;
    while (!tpm.shutdown_req) {

        if (!tpm.rx_active && !tpm.tx_active) {
            {
                std::unique_lock lk(tpm.cv_mutex);
                // spurious wakeups are acceptable
                tpm.cv.wait(lk,
                    [] { return 1; });
            }
        }


        if (tpm.rx_active) {

            struct librfnm_rx_buf* buf;

            {
                std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.in_mutex);

                if (librfnm_rx_s.in.empty()) {
                    goto skip_rx;
                }


                buf = librfnm_rx_s.in.front();
                librfnm_rx_s.in.pop();
            }

            
            libusb_device_handle* lusb_handle = usb_handle->primary;
            if (1 && s->transport_status.usb_boost_connected) {
                std::lock_guard<std::mutex> lockGuard(librfnm_s_transport_pp_mutex);
                //if (s->transport_status.boost_pp_rx) {
                if ((tpm.ep_id % 2) == 0) {
                    lusb_handle = usb_handle->boost;
                }
                s->transport_status.boost_pp_rx = !s->transport_status.boost_pp_rx;
            }

            r = libusb_bulk_transfer(lusb_handle, (((tpm.ep_id % 4) + 1) | LIBUSB_ENDPOINT_IN), (uint8_t*)lrxbuf, RFNM_USB_RX_PACKET_SIZE, &transferred, 1000);
            if (r) {
                printf("RX bulk tx fail %d %d\n", tpm.ep_id, r);
                std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.in_mutex);
                librfnm_rx_s.in.push(buf);
                goto skip_rx;
            }
#if 0

            {
                std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.benchmark_mutex);

                librfnm_rx_s.last_benchmark_adc = !librfnm_rx_s.last_benchmark_adc;

                lrxbuf->adc_id = librfnm_rx_s.last_benchmark_adc;
                //       lrxbuf->adc_id = lrxbuf->adc_id % 2;
                lrxbuf->magic = 0x7ab8bd6f;

                lrxbuf->usb_cc = librfnm_rx_s.usb_cc_benchmark[lrxbuf->adc_id];
                librfnm_rx_s.usb_cc_benchmark[lrxbuf->adc_id]++;
            }
#endif
            if (lrxbuf->magic != 0x7ab8bd6f || lrxbuf->adc_id > 3) {
                //spdlog::error("Wrong magic");
                std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.in_mutex);
                librfnm_rx_s.in.push(buf);
                goto skip_rx;
            }

            if (transferred != RFNM_USB_RX_PACKET_SIZE) {
                printf("thread loop RX usb wrong size, %d, %d\n", transferred, tpm.ep_id);
                std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.in_mutex);
                librfnm_rx_s.in.push(buf);
                goto skip_rx;
            }

            // RFNM_USB_RX_PACKET_ELEM_CNT * LIBRFNM_STREAM_FORMAT_CS16

            if (s->transport_status.rx_stream_format == LIBRFNM_STREAM_FORMAT_CS8) {
                unpack_12_to_cs8(buf->buf, (uint8_t*)lrxbuf->buf, RFNM_USB_RX_PACKET_ELEM_CNT);
            }
            else if (s->transport_status.rx_stream_format == LIBRFNM_STREAM_FORMAT_CS16) {
                unpack_12_to_cs16(buf->buf, (uint8_t*)lrxbuf->buf, RFNM_USB_RX_PACKET_ELEM_CNT);
            }
            else if (s->transport_status.rx_stream_format == LIBRFNM_STREAM_FORMAT_CF32) {
                unpack_12_to_cf32(buf->buf, (uint8_t*)lrxbuf->buf, RFNM_USB_RX_PACKET_ELEM_CNT);
            }
            
            //memcpy(buf->buf, (uint8_t*)lrxbuf, RFNM_USB_RX_PACKET_ELEM_CNT);
            buf->adc_cc = lrxbuf->adc_cc;
            buf->adc_id = lrxbuf->adc_id;
            buf->usb_cc = lrxbuf->usb_cc;
            buf->phytimer = lrxbuf->phytimer;

            {
                std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.out_mutex);
                librfnm_rx_s.out[lrxbuf->adc_id].push(buf);


                //if (librfnm_rx_s.out[lrxbuf->adc_id].size() > 50) {
                librfnm_rx_s.cv.notify_one();
                //}
            }

        }

    skip_rx:

        if (tpm.tx_active) {

            struct librfnm_tx_buf* buf;

            {
                std::lock_guard<std::mutex> lockGuard(librfnm_tx_s.in_mutex);

                if (librfnm_tx_s.in.empty()) {
                    goto read_dev_status;
                }
                buf = librfnm_tx_s.in.front();
                librfnm_tx_s.in.pop();
            }

            pack_cs16_to_12((uint8_t*)ltxbuf->buf, buf->buf, RFNM_USB_TX_PACKET_ELEM_CNT);
            //memcpy((uint8_t*)ltxbuf->buf, buf->buf, RFNM_USB_TX_PACKET_ELEM_CNT);
            ltxbuf->dac_cc = buf->dac_cc;
            ltxbuf->dac_id = buf->dac_id;
            ltxbuf->usb_cc = buf->usb_cc;
            ltxbuf->phytimer = buf->phytimer;
            ltxbuf->magic = 0x758f4d4a;



            libusb_device_handle* lusb_handle = usb_handle->primary;
            if (0 && s->transport_status.usb_boost_connected) {
                std::lock_guard<std::mutex> lockGuard(librfnm_s_transport_pp_mutex);
                if (s->transport_status.boost_pp_tx) {
                    lusb_handle = usb_handle->boost;
                }
                s->transport_status.boost_pp_tx = !s->transport_status.boost_pp_tx;
            }

            r = libusb_bulk_transfer(lusb_handle, (((tpm.ep_id % 4) + 1) | LIBUSB_ENDPOINT_OUT), (uint8_t*)ltxbuf, RFNM_USB_TX_PACKET_SIZE, &transferred, 1000);
            if (r) {
                printf("TX bulk tx fail %d %d", tpm.ep_id, r);
                std::lock_guard<std::mutex> lockGuard(librfnm_tx_s.in_mutex);
                librfnm_tx_s.in.push(buf);
                goto read_dev_status;
            }

            if (transferred != RFNM_USB_TX_PACKET_SIZE) {
                printf("thread loop TX usb wrong size, %d, %d\n", transferred, tpm.ep_id);
                std::lock_guard<std::mutex> lockGuard(librfnm_tx_s.in_mutex);
                librfnm_tx_s.in.push(buf);
                goto read_dev_status;
            }


            {
                std::lock_guard<std::mutex> lockGuard(librfnm_tx_s.out_mutex);
                librfnm_tx_s.out.push(buf);

                librfnm_tx_s.cv.notify_one();
            }


        }

read_dev_status:

        {
            using std::chrono::high_resolution_clock;
            using std::chrono::duration_cast;
            using std::chrono::duration;
            using std::chrono::milliseconds;

            auto tlast = s->last_dev_time;
            auto tnow = high_resolution_clock::now();
            auto ms_int = duration_cast<milliseconds>(tnow - tlast);

            if (ms_int.count() > 5) {
                if (librfnm_s_dev_status_mutex.try_lock())
                {

                    struct rfnm_dev_status dev_status;

                    r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_DEV_STATUS, 0, (unsigned char*)&dev_status, sizeof(struct rfnm_dev_status), 50);
                    if (r < 0) {
                        printf("libusb_control_transfer for RFNM_GET_DEV_STATUS failed\n");
                        //return RFNM_API_USB_FAIL;

                        if (ms_int.count() > 25) {

                            printf("stopping stream\n");

                            for (int8_t i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
                                librfnm_thread_data[i].rx_active = 0;
                                librfnm_thread_data[i].tx_active = 0;
                                librfnm_thread_data[i].shutdown_req = 1;
                            }

                        }
                    }
                    else {
                        memcpy(&s->dev_status, &dev_status, sizeof(struct rfnm_dev_status));
                        s->last_dev_time = high_resolution_clock::now();
                    }


                    librfnm_s_dev_status_mutex.unlock();

                }
            }
        }








    }

    free(lrxbuf);
    free(ltxbuf);
}


MSDLL std::vector<struct rfnm_dev_hwinfo> librfnm::find(enum librfnm_transport transport, std::string address, int bind) {
    if (transport != LIBRFNM_TRANSPORT_USB) {
        printf("Transport not supported\n");
        return {};
    }

    int cnt = 0;
    int dev_cnt = 0;
    int r;
    std::vector<struct rfnm_dev_hwinfo> found;

#if LIBUSB_API_VERSION >= 0x0100010A
    r = libusb_init_context(nullptr, nullptr, 0);
#else
    r = libusb_init(nullptr);
#endif
    if (r < 0) {
        printf("RFNMDevice::activateStream() -> failed to initialize libusb\n");
        return {};
    }

    libusb_device** devs;

    dev_cnt = libusb_get_device_list(NULL, &devs);
    if (dev_cnt < 0) {
        printf("failed to get list of usb devices\n");
        libusb_exit(NULL);
        return {};
    }


    for (int d = 0; d < dev_cnt; d++) {
        struct libusb_device_descriptor desc;
        libusb_device_handle* thandle{};
        int r = libusb_get_device_descriptor(devs[d], &desc);
        if (r < 0) {
            printf("failed to get usb dev descr\n");
            continue;
        }

        if (desc.idVendor != RFNM_USB_VID || desc.idProduct != RFNM_USB_PID) {
            continue;
        }

        r = libusb_open(devs[d], &thandle);
        if (r) {
            printf("Found RFNM device, but couldn't open it %d\n", r);
            continue;
        }

        if (address.length()) {
            auto* sn = new uint8_t[10]();
            if (libusb_get_string_descriptor_ascii(thandle, desc.iSerialNumber, sn, 9) >= 0) {
                sn[8] = '\0';
                if(strcmp((const char*)sn, address.c_str())) {
                    printf("This serial %s doesn't match the requested %s\n", (const char*)sn, address.c_str());
                    libusb_close(thandle);
                    continue;
                }
            }
            else {
                printf("Couldn't read serial descr\n");
                libusb_close(thandle);
                continue;
            }
        }

        if (libusb_get_device_speed(libusb_get_device(thandle)) < LIBUSB_SPEED_SUPER) {
            printf("You are connected using USB 2.0 (480 Mbps), however USB 3.0 (5000 Mbps) is required. Please make sure that the cable and port you are using can work with USB 3.0 SuperSpeed\n");
            libusb_close(thandle);
            continue;
        }

        r = libusb_claim_interface(thandle, 0);
        if (r < 0) {
            printf("Found RFNM device, but couldn't claim the interface, %d, %s\n", r, libusb_strerror(r));
            libusb_close(thandle);
            continue;
        }

        struct rfnm_dev_hwinfo r_hwinfo;

        r = libusb_control_transfer(thandle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_DEV_HWINFO, 0, (unsigned char*)&r_hwinfo, sizeof(struct rfnm_dev_hwinfo), 50);
        if (r < 0) {
            printf("libusb_control_transfer for LIBRFNM_REQ_HWINFO failed\n");
            return {};
        }

        if (r_hwinfo.protocol_version != 1) {
            printf("RFNM_API_SW_UPGRADE_REQUIRED\n");
            return {};
        }

        found.push_back(r_hwinfo);

        libusb_release_interface(thandle, 0);
        libusb_close(thandle);
    }

exit:
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
    return found;
}



MSDLL librfnm::librfnm(enum librfnm_transport transport, std::string address, enum librfnm_debug_level dbg) {

    librfnm_rx_s.qbuf_cnt = 0;

    s = (struct librfnm_status*)calloc(2, sizeof(struct librfnm_status));
    usb_handle = new _librfnm_usb_handle;
    

    if (transport != LIBRFNM_TRANSPORT_USB) {
        printf("Transport not supported\n");
        return;
    }

    int cnt = 0;
    int dev_cnt = 0;
    int r;
    std::vector<struct rfnm_dev_hwinfo> found;

#if LIBUSB_API_VERSION >= 0x0100010A
    r = libusb_init_context(nullptr, nullptr, 0);
#else
    r = libusb_init(nullptr);
#endif
    if (r < 0) {
        printf("RFNMDevice::activateStream() -> failed to initialize libusb\n");
        return;
    }

    libusb_device** devs;

    dev_cnt = libusb_get_device_list(NULL, &devs);
    if (dev_cnt < 0) {
        printf("failed to get list of usb devices\n");
        libusb_exit(NULL);
        return;
    }


    for (int d = 0; d < dev_cnt; d++) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(devs[d], &desc);
        if (r < 0) {
            printf("failed to get usb dev descr\n");
            continue;
        }

        if (desc.idVendor != RFNM_USB_VID || desc.idProduct != RFNM_USB_PID) {
            continue;
        }

        r = libusb_open(devs[d], &usb_handle->primary);
        if (r) {
            printf("Found RFNM device, but couldn't open it %d\n", r);
            continue;
        }

        if (address.length()) {
            auto* sn = new uint8_t[10]();
            if (libusb_get_string_descriptor_ascii(usb_handle->primary, desc.iSerialNumber, sn, 9) >= 0) {
                sn[8] = '\0';
                if (strcmp((const char*)sn, address.c_str())) {
                    printf("This serial %s doesn't match the requested %s\n", (const char*)sn, address.c_str());
                    libusb_close(usb_handle->primary);
                    continue;
                }
            }
            else {
                printf("Couldn't read serial descr\n");
                libusb_close(usb_handle->primary);
                continue;
            }
        }

        if (libusb_get_device_speed(libusb_get_device(usb_handle->primary)) < LIBUSB_SPEED_SUPER) {
            printf("You are connected using USB 2.0 (480 Mbps), however USB 3.0 (5000 Mbps) is required. Please make sure that the cable and port you are using can work with USB 3.0 SuperSpeed\n");
            libusb_close(usb_handle->primary);
            continue;
        }

        r = libusb_claim_interface(usb_handle->primary, 0);
        if (r < 0) {
            printf("Found RFNM device, but couldn't claim the interface, %d, %s\n", r, libusb_strerror(r));
            libusb_close(usb_handle->primary);
            continue;
        }

        s->transport_status.theoretical_mbps = 3500;

        usb_handle->boost = libusb_open_device_with_vid_pid(nullptr, RFNM_USB_VID, RFNM_USB_PID_BOOST);
        if (usb_handle->boost) {
            if (libusb_get_device_speed(libusb_get_device(usb_handle->boost)) >= LIBUSB_SPEED_SUPER) {
                r = libusb_claim_interface(usb_handle->boost, 0);
                if (r >= 0) {
                    s->transport_status.theoretical_mbps += 3500;
                    s->transport_status.usb_boost_connected = 1;
                }
            }
        }

        printf("Max theoretical transport speed is %d Mbps\n", s->transport_status.theoretical_mbps);

        r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_SM_RESET, 0, NULL, 0, 500);
        if (r < 0) {
            printf("Couldn't reset state machine\n");
            return;
        }

        if (get(LIBRFNM_REQ_ALL)) {
            return;
        }

        libusb_free_device_list(devs, 1);


        for (int8_t i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
            librfnm_thread_data[i].ep_id = i + 1;
            librfnm_thread_data[i].rx_active = 0;
            librfnm_thread_data[i].tx_active = 0;
            librfnm_thread_data[i].shutdown_req = 0;
        }



        for (int i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
            librfnm_thread_c[i] = std::thread(&librfnm::threadfn, this, i);
        }



        return;

    }

exit:
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
    return;







#if 0
    for (int8_t i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
        librfnm_thread_data[i].ep_id = i + 1;
        librfnm_thread_data[i].rx_active = 0;
        librfnm_thread_data[i].tx_active = 0;
        librfnm_thread_data[i].shutdown_req = 0;
    }



    for (int i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
        librfnm_thread_c[i] = std::thread(threadfn, i);
    }
#endif

#if 0

    s->rx.ch[0].enable = RFNM_CH_ON;
    s->rx.ch[0].freq = RFNM_MHZ_TO_HZ(2500);
    s->rx.ch[0].path = s->rx.ch[0].path_preferred;
    s->rx.ch[0].samp_freq_div_n = 2;

    s->rx.ch[1].enable = RFNM_CH_ON;
    s->rx.ch[1].freq = RFNM_MHZ_TO_HZ(2500);
    s->rx.ch[1].path = s->rx.ch[1].path_preferred;
    s->tx.ch[1].samp_freq_div_n = 2;

    //s->tx.ch[0].enable = RFNM_CH_ON;
    //s->tx.ch[0].freq = RFNM_MHZ_TO_HZ(2500);
    //s->tx.ch[0].path = s->tx.ch[0].path_preferred;
    //s->tx.ch[0].samp_freq_div_n = 2;
     
    volatile rfnm_api_failcode fail;
    fail = librfnm_set(LIBRFNM_APPLY_CH0_RX /*| LIBRFNM_APPLY_CH0_TX  | LIBRFNM_APPLY_CH1_RX*/);
    int outbufsize, inbufsize;
    librfnm_rx_stream(LIBRFNM_STREAM_FORMAT_CS16, &outbufsize);
    librfnm_tx_stream(LIBRFNM_STREAM_FORMAT_CS16, &inbufsize);
    struct librfnm_rx_buf rxbuf[NBUF];
    //struct librfnm_tx_buf txbuf[NBUF];

    std::queue<struct librfnm_tx_buf*> ltxqueue;
    //std::queue<struct librfnm_rx_buf*> lrxqueue;

    for (int i = 0; i < NBUF; i++) {
        rxbuf[i].buf = (uint8_t*)malloc(outbufsize);
        librfnm_rx_qbuf(&rxbuf[i]);
        //txbuf[i].buf = rxbuf[i].buf;
        //txbuf[i].buf = (uint8_t*)malloc(inbufsize);

        //ltxqueue.push(&txbuf[i]);
    }

    spdlog::info("outbufsize {} inbufsize {} ", outbufsize, inbufsize);

    while (1) {
        struct librfnm_rx_buf* lrxbuf;
        struct librfnm_tx_buf* ltxbuf;
        rfnm_api_failcode err;
        
        while (!librfnm_rx_dqbuf(&lrxbuf, LIBRFNM_CH0 /* | LIBRFNM_CH1*/, 0)) {
            
            //ltxbuf = (struct librfnm_tx_buf*) calloc(1, sizeof(struct librfnm_tx_buf));
            //ltxbuf->buf = (uint8_t*) malloc(outbufsize);
            //memcpy(ltxbuf->buf, lrxbuf->buf, inbufsize);
            //ltxqueue.push(ltxbuf);

#if 0

            static int delay_wavedetct = 0;
            static int wavedetect = 0;

            if (1 || ++delay_wavedetct == 10) {
                delay_wavedetct = 0;

                for (int s = 0; s < 16000; s += 64) {



                    int16_t* q, * i;
                    uint32_t* t;

                    q = (int16_t*)&ltxbuf->buf[s];
                    t = (uint32_t*)&ltxbuf->buf[s];

                    int16_t li, lq;

                    li = (0xffff0000 & (*t)) >> 16;
                    lq = *q;

                    li = abs(li);
                    lq = abs(lq);

#if 1
                    if ((li + lq) > (150 << 4)) {
                        if (!wavedetect) {
                            wavedetect = 1;
                            printf("yes %d\t%d %d\n", lq, li);
                        }

                    }
                    else if ((li + lq) < (20 << 4)) {
                        if (wavedetect) {
                            wavedetect = 0;
                            printf("no %d\t%d %d\n", lq, li);
                        }
                    }
#else
                    printf("%d %d\n", li, lq);
#endif

                }
    }



#endif


            librfnm_rx_qbuf(lrxbuf);
        }

        while (ltxqueue.size()) {
            ltxbuf = ltxqueue.front();

#if 0
            uint16_t* d;
            static double t;
            static int dd = 0;
            dd++;

            d = (uint16_t*)ltxbuf->buf;
            for (int q = 0; q + 1 < (outbufsize / 2); q += 2) {

                

                if (dd >= 1) {
                    d[q] = (int16_t)0;
                    if (dd == 2) {
                        dd = 0;
                    }
                }
                else {
                    d[q] = (int16_t) int(0x8000 * sin(t) + 0);
                    t += (2 * 3.1415) / 300;
                }
                //d[q+1] = d[q];
                
            }
#endif


            if (!librfnm_tx_qbuf(ltxbuf)) {
                ltxqueue.pop();
            }
            else {
                break;
            }
        }

        while (!librfnm_tx_dqbuf(&ltxbuf)) {
            free(ltxbuf->buf);
            free(ltxbuf);
        }


        
    }


    for (auto& i : librfnm_thread_c) {
            i.join();
    }

#endif

    return;
    //return (struct librfnm_status*)s;
    //spdlog::info("librfnm_init ok");
}

MSDLL librfnm::~librfnm() {

    for (int8_t i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
        librfnm_thread_data[i].rx_active = 0;
        librfnm_thread_data[i].tx_active = 0;
        librfnm_thread_data[i].shutdown_req = 1;
    }

    for (auto& i : librfnm_thread_c) {
        i.join();
    }

    if (usb_handle) {
        if (usb_handle->primary) {
            libusb_close(usb_handle->primary);
        }
        if (usb_handle->boost) {
            libusb_close(usb_handle->boost);
        }
        delete usb_handle;
    }
}
#if 0
int librfnm::format_to_bytes_per_ele(enum librfnm_stream_format format) {
    switch (format) {
    case LIBRFNM_STREAM_FORMAT_CS8:
        return 2;
        break;
    case LIBRFNM_STREAM_FORMAT_CS16:
        return 4;
        break;
    case LIBRFNM_STREAM_FORMAT_CF32:
        return 8;
        break;
    default:
        return 0;
    }
}
#endif

MSDLL rfnm_api_failcode librfnm::rx_stream(enum librfnm_stream_format format, int* bufsize) {
    rfnm_api_failcode ret = RFNM_API_OK;
    
    switch (format) {
    case LIBRFNM_STREAM_FORMAT_CS8:
        s->transport_status.rx_stream_format = format;
        *bufsize = RFNM_USB_RX_PACKET_ELEM_CNT * format;
        break;
    case LIBRFNM_STREAM_FORMAT_CS16:
        s->transport_status.rx_stream_format = format;
        *bufsize = RFNM_USB_RX_PACKET_ELEM_CNT * format;
        break;
    case LIBRFNM_STREAM_FORMAT_CF32:
        s->transport_status.rx_stream_format = format;
        *bufsize = RFNM_USB_RX_PACKET_ELEM_CNT * format;
        break;
    default: 
        return RFNM_API_NOT_SUPPORTED;
    }



    for (int8_t i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
        std::lock_guard<std::mutex> lockGuard(librfnm_thread_data[i].cv_mutex);
        librfnm_thread_data[i].rx_active = 1;
        librfnm_thread_data[i].cv.notify_one();
    }

    return ret;
}

MSDLL rfnm_api_failcode librfnm::tx_stream(enum librfnm_stream_format format, int* bufsize, enum librfnm_tx_latency_policy policy) {
    rfnm_api_failcode ret = RFNM_API_OK;

    switch (format) {
    case LIBRFNM_STREAM_FORMAT_CS8:
        s->transport_status.tx_stream_format = format;
        *bufsize = RFNM_USB_RX_PACKET_ELEM_CNT * 2;
        break;
    case LIBRFNM_STREAM_FORMAT_CS16:
        s->transport_status.tx_stream_format = format;
        *bufsize = RFNM_USB_RX_PACKET_ELEM_CNT * 4;
        break;
    case LIBRFNM_STREAM_FORMAT_CF32:
        s->transport_status.tx_stream_format = format;
        *bufsize = RFNM_USB_RX_PACKET_ELEM_CNT * 8;
        break;
    default:
        return RFNM_API_NOT_SUPPORTED;
    }

    for (int8_t i = 0; i < LIBRFNM_THREAD_COUNT; i++) {
        std::lock_guard<std::mutex> lockGuard(librfnm_thread_data[i].cv_mutex);
        librfnm_thread_data[i].tx_active = 1;
        librfnm_thread_data[i].cv.notify_one();
    }

    return ret;
}

MSDLL rfnm_api_failcode librfnm::rx_qbuf(struct librfnm_rx_buf * buf) {

    librfnm_rx_s.qbuf_cnt++;
    
    std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.in_mutex);

    librfnm_rx_s.in.push(buf);
    return RFNM_API_OK;
}

MSDLL rfnm_api_failcode librfnm::tx_qbuf(struct librfnm_tx_buf* buf, uint32_t wait_for_ms) {

    //std::lock_guard<std::mutex> lockGuard1(librfnm_tx_s.cc_mutex);

    std::lock_guard<std::mutex> lockGuard1(librfnm_s_dev_status_mutex);

    if (librfnm_tx_s.usb_cc - s->dev_status.usb_dac_last_dqbuf > 100) {
        return RFNM_API_MIN_QBUF_QUEUE_FULL;
    }

    std::lock_guard<std::mutex> lockGuard2(librfnm_tx_s.in_mutex);

    librfnm_tx_s.qbuf_cnt++;
    librfnm_tx_s.usb_cc++;

    buf->usb_cc = (uint32_t)librfnm_tx_s.usb_cc;

    librfnm_tx_s.in.push(buf);
    return RFNM_API_OK;
}

MSDLL int librfnm::single_ch_id_bitmap_to_adc_id(uint8_t ch_ids) {
    int ch_id = 0;
    while (ch_id < 8) {
        if ((ch_ids & 0x1) == 1) {
            return s->rx.ch[ch_id].adc_id;
        }
        ch_id++;
        ch_ids = ch_ids >> 1;
    }
    return -1;
}

MSDLL void librfnm::dqbuf_overwrite_cc(uint8_t adc_id, int acquire_lock) {

    struct librfnm_rx_buf* buf;

    if (acquire_lock) {
        librfnm_rx_s.out_mutex.lock();
    }
    librfnm_rx_s.in_mutex.lock();
#if 1
    if (librfnm_rx_s.out[adc_id].size()) {

        int size = librfnm_rx_s.out[adc_id].size();

        for (int i = 0; i < /*size / 2*/1; i++) {

            buf = librfnm_rx_s.out[adc_id].top();
            librfnm_rx_s.usb_cc[adc_id] = buf->usb_cc + 1;
            librfnm_rx_s.in.push(buf);
            librfnm_rx_s.out[adc_id].pop();

            //spdlog::info("new cc {}", librfnm_rx_s.usb_cc[adc_id]);

        }
    }
#else
    if (librfnm_rx_s.out[adc_id].size()) {

        buf = librfnm_rx_s.out[adc_id].top();
        librfnm_rx_s.usb_cc[adc_id] = buf->usb_cc + 1;
        librfnm_rx_s.in.push(buf);
        librfnm_rx_s.out[adc_id].pop();

        spdlog::info("new cc {}", librfnm_rx_s.usb_cc[adc_id]);

        int drop_cnt = 0;
        int save_cnt = 0;

        std::priority_queue<struct librfnm_rx_buf*, std::vector<struct librfnm_rx_buf*>, librfnm_rx_buf_compare> tq;

        while (librfnm_rx_s.out[adc_id].size()) {
            buf = librfnm_rx_s.out[adc_id].top();
            if (buf->usb_cc < librfnm_rx_s.usb_cc[adc_id]) {
                librfnm_rx_s.in.push(buf);

                drop_cnt++;
            }
            else {
                tq.push(buf);
                //     spdlog::info("saving {}", buf->usb_cc);
                save_cnt++;
            }


            librfnm_rx_s.out[adc_id].pop();
        }

        spdlog::info("dropping {} saving {}", drop_cnt, save_cnt);

        while (tq.size()) {
            buf = tq.top();
            librfnm_rx_s.out[adc_id].push(buf);
            tq.pop();
        }
    }
#endif
    //spdlog::info("new cc is {}", librfnm_rx_s.usb_cc[adc_id]);

    librfnm_rx_s.in_mutex.unlock();
    if (acquire_lock) {
        librfnm_rx_s.out_mutex.unlock();
    }
}

MSDLL int librfnm::dqbuf_is_cc_continuous(uint8_t adc_id, int acquire_lock) {
    //return librfnm_rx_s.out[adc_id].size() > 50;

    struct librfnm_rx_buf* buf;
    int queue_size = 0;

    
    if (acquire_lock) {
        librfnm_rx_s.out_mutex.lock();
    }
    queue_size = librfnm_rx_s.out[adc_id].size();
    if (queue_size < 5) {
        if (acquire_lock) {
            librfnm_rx_s.out_mutex.unlock();
        }
        return 0;
    }
    buf = librfnm_rx_s.out[librfnm_rx_s.required_adc_id].top();
    if (acquire_lock) {
        librfnm_rx_s.out_mutex.unlock();
    }
    
    

    if (librfnm_rx_s.usb_cc[adc_id] == buf->usb_cc) {
        return 1;
    }
    else {
        if (acquire_lock && queue_size > LIBRFNM_RX_RECOMB_BUF_LEN) {
        //if (queue_size > LIBRFNM_RX_RECOMB_BUF_LEN) {
            printf("cc %llu overwritten at queue size %d adc %d\n", librfnm_rx_s.usb_cc[librfnm_rx_s.required_adc_id], queue_size, librfnm_rx_s.required_adc_id);
            dqbuf_overwrite_cc(adc_id, acquire_lock);
        }
        return 0;
    }
}

MSDLL rfnm_api_failcode librfnm::rx_dqbuf(struct librfnm_rx_buf ** buf, uint8_t ch_ids, uint32_t wait_for_ms) {
    int is_single_ch, required_adc_id;

    if (librfnm_rx_s.qbuf_cnt < LIBRFNM_MIN_RX_BUFCNT) {
        return RFNM_API_MIN_QBUF_CNT_NOT_SATIFIED;
    }
 
    switch (ch_ids) {
    case LIBRFNM_CH0:
    case LIBRFNM_CH1:
    case LIBRFNM_CH2:
    case LIBRFNM_CH3:
    case LIBRFNM_CH4:
    case LIBRFNM_CH5:
    case LIBRFNM_CH6:
    case LIBRFNM_CH7:
        is_single_ch = 1;
        break;
    case 0:
        is_single_ch = 0;
        ch_ids = 0xff;
        break;
    default: 
        is_single_ch = 0;
        break;
    }

    if (is_single_ch) {
        required_adc_id = single_ch_id_bitmap_to_adc_id(ch_ids);
    }
    else {
        static int last_dqbuf_ch = 0;
        
        do {
            uint8_t mask = (1 << last_dqbuf_ch);
            required_adc_id = single_ch_id_bitmap_to_adc_id(ch_ids & mask);
       
            if (++last_dqbuf_ch == 8) {
                last_dqbuf_ch = 0;
            }
        } while (required_adc_id < 0);
    }

    librfnm_rx_s.required_adc_id = required_adc_id;
    
    if(!dqbuf_is_cc_continuous(librfnm_rx_s.required_adc_id, 1)) {

        if (!wait_for_ms) {
            return RFNM_API_DQBUF_NO_DATA;
        }

        {
            std::unique_lock lk(librfnm_rx_s.out_mutex);
            librfnm_rx_s.cv.wait_for(lk, std::chrono::milliseconds(wait_for_ms),
                [this] { return dqbuf_is_cc_continuous(librfnm_rx_s.required_adc_id, 0) || librfnm_rx_s.out[librfnm_rx_s.required_adc_id].size() > LIBRFNM_RX_RECOMB_BUF_LEN; }
            );
        }
        
        if (!dqbuf_is_cc_continuous(librfnm_rx_s.required_adc_id, 1)) {
            printf("cc timeout %llu\n", librfnm_rx_s.usb_cc[librfnm_rx_s.required_adc_id]);

            if (librfnm_rx_s.out[librfnm_rx_s.required_adc_id].size()) {
                dqbuf_overwrite_cc(librfnm_rx_s.required_adc_id, 1);
            }
            
            return RFNM_API_DQBUF_NO_DATA;
        }
    }
    

    //spdlog::info("len {}", librfnm_rx_s.out.size());

    //while(!librfnm_rx_s.out.size()) {}

    //spdlog::info("len {}", librfnm_rx_s.out.size());

    {
        std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.out_mutex);
        *buf = librfnm_rx_s.out[librfnm_rx_s.required_adc_id].top();
        librfnm_rx_s.out[librfnm_rx_s.required_adc_id].pop();
    }
    

    struct librfnm_rx_buf* lb;
    lb = *buf;

    librfnm_rx_s.usb_cc[librfnm_rx_s.required_adc_id]++;

    if ((lb->usb_cc & 0xff) < 0x10) {
    //    std::lock_guard<std::mutex> lockGuard(librfnm_rx_s.out_mutex);
    //    spdlog::info("cc {} {} {}", lb->usb_cc, lcc, librfnm_rx_s.out.size());
    }

    //spdlog::info("cc {} adc {}", lb->usb_cc, lb->adc_id);

    return RFNM_API_OK;
}

MSDLL enum rfnm_rf_path librfnm::string_to_rf_path(std::string path) {

    std::transform(path.begin(), path.end(), path.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (!path.compare("embed") || !path.compare("emb") || !path.compare("embedded") || !path.compare("internal") || !path.compare("onboard")) {
        return RFNM_PATH_EMBED_ANT;
    }

    if (!path.compare("loop") || !path.compare("loopback")) {
        return RFNM_PATH_LOOPBACK;
    }
    
    if (path.find("sma") != std::string::npos) {
        path.replace(path.find("sma"), 3, "");
    }

    if (path.find("ant") != std::string::npos) {
        path.replace(path.find("ant"), 3, "");
    }

    if (path.find("-") != std::string::npos) {
        path.replace(path.find("-"), 1, "");
    }

    if (path.find("_") != std::string::npos) {
        path.replace(path.find("_"), 1, "");
    }

    if (path.find(" ") != std::string::npos) {
        path.replace(path.find(" "), 1, "");
    }

    if (path.length() != 1 || path.c_str()[0] < 'a' || path.c_str()[0] > 'h') {
        return RFNM_PATH_NULL;
    }


    return (enum rfnm_rf_path) (path.c_str()[0] - 'a');
}

MSDLL std::string librfnm::rf_path_to_string(enum rfnm_rf_path path) {

    if (path == RFNM_PATH_NULL) {
        return "null";
    }
    else if (path == RFNM_PATH_EMBED_ANT) {
        return "embed";
    }
    else if (path == RFNM_PATH_LOOPBACK) {
        return "loopback";
    }
    else {
        return std::string(1, 'A' + (int)(path));
    }
}


MSDLL rfnm_api_failcode librfnm::tx_dqbuf(struct librfnm_tx_buf** buf) {

    std::lock_guard<std::mutex> lockGuard(librfnm_tx_s.out_mutex);
    
    if (librfnm_tx_s.out.size()) {
        *buf = librfnm_tx_s.out.front();
        librfnm_tx_s.out.pop();
        return RFNM_API_OK;
    }
    else {
        return RFNM_API_DQBUF_NO_DATA;
    }
  
}





MSDLL rfnm_api_failcode librfnm::get(enum librfnm_req_type type) {
    int r;

    if (type & LIBRFNM_REQ_HWINFO) {
        struct rfnm_dev_hwinfo r_hwinfo;
        //spdlog::info("RFNMDevice::activateStream() -> rfnm_dev_hwinfo is {} bytes", sizeof(struct rfnm_dev_hwinfo));

        r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_DEV_HWINFO, 0, (unsigned char*)&r_hwinfo, sizeof(struct rfnm_dev_hwinfo), 50);
        if (r < 0) {
            printf("libusb_control_transfer for LIBRFNM_REQ_HWINFO failed\n");
            return RFNM_API_USB_FAIL;
        }
        memcpy(&s->hwinfo, &r_hwinfo, sizeof(struct rfnm_dev_hwinfo));

        if (r_hwinfo.protocol_version != 1) {
            printf("RFNM_API_SW_UPGRADE_REQUIRED\n");
            return RFNM_API_SW_UPGRADE_REQUIRED;
        }
    }

    if (type & LIBRFNM_REQ_TX) {
        struct rfnm_dev_tx_ch_list r_chlist;
        //spdlog::info("RFNMDevice::activateStream() -> rfnm_dev_tx_ch_list is {} bytes", sizeof(struct rfnm_dev_tx_ch_list));

        r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_TX_CH_LIST, 0, (unsigned char*)&r_chlist, sizeof(struct rfnm_dev_tx_ch_list), 50);
        if (r < 0) {
            printf("libusb_control_transfer for LIBRFNM_REQ_TX failed\n");
            return RFNM_API_USB_FAIL;
        }
        memcpy(&s->tx, &r_chlist, sizeof(struct rfnm_dev_tx_ch_list));
    }

    if (type & LIBRFNM_REQ_RX) {
        struct rfnm_dev_rx_ch_list r_chlist;
        //spdlog::info("RFNMDevice::activateStream() -> rfnm_dev_rx_ch_list is {} bytes", sizeof(struct rfnm_dev_rx_ch_list));

        r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_RX_CH_LIST, 0, (unsigned char*)&r_chlist, sizeof(struct rfnm_dev_rx_ch_list), 50);
        if (r < 0) {
            printf("libusb_control_transfer for LIBRFNM_REQ_RX failed\n");
            return RFNM_API_USB_FAIL;
        }
        memcpy(&s->rx, &r_chlist, sizeof(struct rfnm_dev_rx_ch_list));
    }

    if (type & LIBRFNM_REQ_DEV_STATUS) {
        struct rfnm_dev_status dev_status;

        r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_DEV_STATUS, 0, (unsigned char*)&dev_status, sizeof(struct rfnm_dev_status), 50);
        if (r < 0) {
            printf("libusb_control_transfer for RFNM_GET_DEV_STATUS failed %d\n", r);
            return RFNM_API_USB_FAIL;
        }
        memcpy(&s->dev_status, &dev_status, sizeof(struct rfnm_dev_status));
    }

    return RFNM_API_OK;
}

MSDLL rfnm_api_failcode librfnm::set(uint16_t applies, bool confirm_execution, uint32_t wait_for_ms) {
    int r;
    volatile static uint32_t cc_tx = 0;
    volatile static uint32_t cc_rx = 0;

    uint8_t applies_ch_tx = applies & 0xff;
    uint8_t applies_ch_rx = (applies & 0xff00) >> 8;

    if (applies_ch_tx) {
        struct rfnm_dev_tx_ch_list r_chlist;
        memcpy(&r_chlist, &s->tx, sizeof(struct rfnm_dev_tx_ch_list));
        r_chlist.apply = applies_ch_tx;
        r_chlist.cc = ++cc_tx;

        r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_SET_TX_CH_LIST, 0, (unsigned char*)&r_chlist, sizeof(struct rfnm_dev_tx_ch_list), 50);
        if (r < 0) {
            printf("libusb_control_transfer for LIBRFNM_REQ_TX failed\n");
            return RFNM_API_USB_FAIL;
        }
    }

    if (applies_ch_rx) {
        struct rfnm_dev_rx_ch_list r_chlist;
        memcpy(&r_chlist, &s->rx, sizeof(struct rfnm_dev_rx_ch_list));
        r_chlist.apply = applies_ch_rx;
        r_chlist.cc = ++cc_rx;

        r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_SET_RX_CH_LIST, 0, (unsigned char*)&r_chlist, sizeof(struct rfnm_dev_rx_ch_list), 50);
        if (r < 0) {
            printf("libusb_control_transfer for LIBRFNM_REQ_RX failed\n");
            return RFNM_API_USB_FAIL;
        }
    }

    if (confirm_execution) {
        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::duration;
        using std::chrono::milliseconds;

        auto tstart = high_resolution_clock::now();

        while (1) {
            struct rfnm_dev_get_set_result r_res;

            r = libusb_control_transfer(usb_handle->primary, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, RFNM_B_REQUEST, RFNM_GET_SET_RESULT, 0, (unsigned char*)&r_res, sizeof(struct rfnm_dev_get_set_result), 50);
            if (r < 0) {
                printf("libusb_control_transfer for LIBRFNM_REQ_RX failed\n");
                return RFNM_API_USB_FAIL;
            }

            if (r_res.cc_rx == cc_rx && r_res.cc_tx == cc_tx) {
                for (int q = 0; q < 8; q++) {
                    if (((1 << q) & applies_ch_tx) && r_res.tx_ecodes[q]) {
                        return (rfnm_api_failcode) r_res.tx_ecodes[q];
                    }
                }
                for (int q = 0; q < 8; q++) {
                    if (((1 << q) & applies_ch_rx) && r_res.rx_ecodes[q]) {
                        return (rfnm_api_failcode) r_res.rx_ecodes[q];
                    }
                }
                return RFNM_API_OK;
            }

            auto tnow = high_resolution_clock::now();
            auto ms_int = duration_cast<milliseconds>(tnow - tstart);
            
            if (ms_int.count() > wait_for_ms) {
                return RFNM_API_TIMEOUT;
            }
        }
    }
    return RFNM_API_OK;
}