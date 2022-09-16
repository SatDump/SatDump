#include <string.h>

#include "FX3handler.h"
#include "usb_device.h"
#include <stdlib.h>
#include <stdio.h>

fx3class *CreateUsbHandler()
{
    return new fx3handler();
}

fx3handler::fx3handler()
{
    readsize = 0;
    fill = 0;
    run = false;
}

fx3handler::~fx3handler()
{
    StopStream();
    if (dev)
        usb_device_close(dev);
}

bool fx3handler::Open(uint8_t *fw_data, uint32_t fw_size)
{
    dev = usb_device_open(0, (const char *)fw_data, fw_size);
    return dev != nullptr;
}

bool fx3handler::Control(FX3Command command, uint8_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint32_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::Control(FX3Command command, uint64_t data)
{
    return usb_device_control(this->dev, command, 0, 0, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::SetArgument(uint16_t index, uint16_t value)
{
    uint8_t data = 0;
    return usb_device_control(this->dev, SETARGFX3, value, index, (uint8_t *)&data, sizeof(data), 0) == 0;
}

bool fx3handler::GetHardwareInfo(uint32_t *data)
{
    return usb_device_control(this->dev, TESTFX3, 0, 0, (uint8_t *)data, sizeof(*data), 1) == 0;
}

void fx3handler::StartStream(ringbuffer<int16_t> &input, int numofblock)
{
    fprintf(stderr, "fx3handler::StartStream()\n");
    inputbuffer = &input;
    readsize = input.getBlockSize() * sizeof(uint16_t);
    fill = 0;
    stream = streaming_open_async(this->dev, readsize, numofblock, PacketRead, this);

    // Start background thread to poll the events
    run = true;
    poll_thread = std::thread(
        [this]()
        {
            while (run)
            {
                usb_device_handle_events(this->dev);
                // printf("USB HANDLER\r\n");
            }
        });

    if (stream)
    {
        streaming_start(stream);
    }
}

void fx3handler::StopStream()
{
    fprintf(stderr, "fx3handler::StopStream()\n");
    if (!run)
        return;
    run = false;
    poll_thread.join();
    fprintf(stderr, "fx3handler::StopStream() thread joined\n");
    if (stream)
    {
        streaming_stop(stream);
        fprintf(stderr, "fx3handler::StopStream() streaming_stop\n");
        streaming_close(stream);
        fprintf(stderr, "fx3handler::StopStream() streaming_close\n");
        stream = nullptr;
    }
}

void fx3handler::PacketRead(uint32_t data_size, uint8_t *data, void *context)
{
    fx3handler *handler = (fx3handler *)context;

    uint32_t written = 0;
    while (written < data_size)
    {
        uint8_t *ptr = (uint8_t *)handler->inputbuffer->getWritePtr();
        int to_copy = std::min(int(data_size - written), handler->readsize - handler->fill);
        memcpy(&ptr[handler->fill], &data[written], to_copy);
        handler->fill += to_copy;
        written += to_copy;
        if (handler->fill >= handler->readsize)
        {
            handler->inputbuffer->WriteDone();
            handler->fill = 0;
        }
    }
}

bool fx3handler::ReadDebugTrace(uint8_t *pdata, uint8_t len)
{
    return true;
}

bool fx3handler::Enumerate(unsigned char &idx, char *lbuf, uint8_t *fw_data, uint32_t fw_size)
{
    return true; // TBD
}
