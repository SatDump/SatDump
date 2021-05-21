#pragma once
#include <mutex>
#include <condition_variable>
#include <volk/volk.h>
#include <string.h>

namespace dsp
{
// 1MB buffer
#define STREAM_BUFFER_SIZE 1000000

    template <class T>
    class stream
    {
    public:
        stream()
        {
            writeBuf = (T *)volk_malloc(STREAM_BUFFER_SIZE * sizeof(T), volk_get_alignment());
            readBuf = (T *)volk_malloc(STREAM_BUFFER_SIZE * sizeof(T), volk_get_alignment());
        }

        ~stream()
        {
            volk_free(writeBuf);
            volk_free(readBuf);
        }

        bool swap(int size)
        {
            {
                // Wait to either swap or stop
                std::unique_lock<std::mutex> lck(swapMtx);
                swapCV.wait(lck, [this] { return (canSwap || writerStop); });

                // If writer was stopped, abandon operation
                if (writerStop)
                {
                    return false;
                }

                // Swap buffers
                dataSize = size;
                T *temp = writeBuf;
                writeBuf = readBuf;
                readBuf = temp;
                canSwap = false;
            }

            // Notify reader that some data is ready
            {
                std::lock_guard<std::mutex> lck(rdyMtx);
                dataReady = true;
            }
            rdyCV.notify_all();

            return true;
        }

        int read()
        {
            // Wait for data to be ready or to be stopped
            std::unique_lock<std::mutex> lck(rdyMtx);
            rdyCV.wait(lck, [this] { return (dataReady || readerStop); });

            return (readerStop ? -1 : dataSize);
        }

        void flush()
        {
            // Clear data ready
            {
                std::lock_guard<std::mutex> lck(rdyMtx);
                dataReady = false;
            }

            // Notify writer that buffers can be swapped
            {
                std::lock_guard<std::mutex> lck(swapMtx);
                canSwap = true;
            }

            swapCV.notify_all();
        }

        void stopWriter()
        {
            {
                std::lock_guard<std::mutex> lck(swapMtx);
                writerStop = true;
            }
            swapCV.notify_all();
        }

        void clearWriteStop()
        {
            writerStop = false;
        }

        void stopReader()
        {
            {
                std::lock_guard<std::mutex> lck(rdyMtx);
                readerStop = true;
            }
            rdyCV.notify_all();
        }

        void clearReadStop()
        {
            readerStop = false;
        }

        T *writeBuf;
        T *readBuf;

    private:
        std::mutex swapMtx;
        std::condition_variable swapCV;
        bool canSwap = true;

        std::mutex rdyMtx;
        std::condition_variable rdyCV;
        bool dataReady = false;

        bool readerStop = false;
        bool writerStop = false;

        int dataSize = 0;
    };

#define RING_BUF_SZ 1000000

    template <class T>
    class RingBuffer
    {
    public:
        RingBuffer()
        {
        }

        RingBuffer(int maxLatency) { init(maxLatency); }

        ~RingBuffer() { delete _buffer; }

        void init(int maxLatency)
        {
            size = RING_BUF_SZ;
            _buffer = new T[size];
            _stopReader = false;
            _stopWriter = false;
            this->maxLatency = maxLatency;
            writec = 0;
            readc = 0;
            readable = 0;
            writable = size;
            memset(_buffer, 0, size * sizeof(T));
        }

        int read(T *data, int len)
        {
            int dataRead = 0;
            int toRead = 0;
            while (dataRead < len)
            {
                toRead = std::min<int>(waitUntilReadable(), len - dataRead);
                if (toRead < 0)
                {
                    return -1;
                };

                if ((toRead + readc) > size)
                {
                    memcpy(&data[dataRead], &_buffer[readc], (size - readc) * sizeof(T));
                    memcpy(&data[dataRead + (size - readc)], &_buffer[0], (toRead - (size - readc)) * sizeof(T));
                }
                else
                {
                    memcpy(&data[dataRead], &_buffer[readc], toRead * sizeof(T));
                }

                dataRead += toRead;

                _readable_mtx.lock();
                readable -= toRead;
                _readable_mtx.unlock();
                _writable_mtx.lock();
                writable += toRead;
                _writable_mtx.unlock();
                readc = (readc + toRead) % size;
                canWriteVar.notify_one();
            }
            return len;
        }

        int readAndSkip(T *data, int len, int skip)
        {
            int dataRead = 0;
            int toRead = 0;
            while (dataRead < len)
            {
                toRead = std::min<int>(waitUntilReadable(), len - dataRead);
                if (toRead < 0)
                {
                    return -1;
                };

                if ((toRead + readc) > size)
                {
                    memcpy(&data[dataRead], &_buffer[readc], (size - readc) * sizeof(T));
                    memcpy(&data[dataRead + (size - readc)], &_buffer[0], (toRead - (size - readc)) * sizeof(T));
                }
                else
                {
                    memcpy(&data[dataRead], &_buffer[readc], toRead * sizeof(T));
                }

                dataRead += toRead;

                _readable_mtx.lock();
                readable -= toRead;
                _readable_mtx.unlock();
                _writable_mtx.lock();
                writable += toRead;
                _writable_mtx.unlock();
                readc = (readc + toRead) % size;
                canWriteVar.notify_one();
            }
            dataRead = 0;
            while (dataRead < skip)
            {
                toRead = std::min<int>(waitUntilReadable(), skip - dataRead);
                if (toRead < 0)
                {
                    return -1;
                };

                dataRead += toRead;

                _readable_mtx.lock();
                readable -= toRead;
                _readable_mtx.unlock();
                _writable_mtx.lock();
                writable += toRead;
                _writable_mtx.unlock();
                readc = (readc + toRead) % size;
                canWriteVar.notify_one();
            }
            return len;
        }

        int waitUntilReadable()
        {
            if (_stopReader)
            {
                return -1;
            }
            int _r = getReadable();
            if (_r != 0)
            {
                return _r;
            }
            std::unique_lock<std::mutex> lck(_readable_mtx);
            canReadVar.wait(lck, [=]() { return ((this->getReadable(false) > 0) || this->getReadStop()); });
            if (_stopReader)
            {
                return -1;
            }
            return getReadable(false);
        }

        int getReadable(bool lock = true)
        {
            if (lock)
            {
                _readable_mtx.lock();
            };
            int _r = readable;
            if (lock)
            {
                _readable_mtx.unlock();
            };
            return _r;
        }

        int write(T *data, int len)
        {
            int dataWritten = 0;
            int toWrite = 0;
            while (dataWritten < len)
            {
                toWrite = std::min<int>(waitUntilwritable(), len - dataWritten);
                if (toWrite < 0)
                {
                    return -1;
                };

                if ((toWrite + writec) > size)
                {
                    memcpy(&_buffer[writec], &data[dataWritten], (size - writec) * sizeof(T));
                    memcpy(&_buffer[0], &data[dataWritten + (size - writec)], (toWrite - (size - writec)) * sizeof(T));
                }
                else
                {
                    memcpy(&_buffer[writec], &data[dataWritten], toWrite * sizeof(T));
                }

                dataWritten += toWrite;

                _readable_mtx.lock();
                readable += toWrite;
                _readable_mtx.unlock();
                _writable_mtx.lock();
                writable -= toWrite;
                _writable_mtx.unlock();
                writec = (writec + toWrite) % size;

                canReadVar.notify_one();
            }
            return len;
        }

        int waitUntilwritable()
        {
            if (_stopWriter)
            {
                return -1;
            }
            int _w = getWritable();
            if (_w != 0)
            {
                return _w;
            }
            std::unique_lock<std::mutex> lck(_writable_mtx);
            canWriteVar.wait(lck, [=]() { return ((this->getWritable(false) > 0) || this->getWriteStop()); });
            if (_stopWriter)
            {
                return -1;
            }
            return getWritable(false);
        }

        int getWritable(bool lock = true)
        {
            if (lock)
            {
                _writable_mtx.lock();
            };
            int _w = writable;
            if (lock)
            {
                _writable_mtx.unlock();
                _readable_mtx.lock();
            };
            int _r = readable;
            if (lock)
            {
                _readable_mtx.unlock();
            };
            return std::max<int>(std::min<int>(_w, maxLatency - _r), 0);
        }

        void stopReader()
        {
            _stopReader = true;
            canReadVar.notify_one();
        }

        void stopWriter()
        {
            _stopWriter = true;
            canWriteVar.notify_one();
        }

        bool getReadStop()
        {
            return _stopReader;
        }

        bool getWriteStop()
        {
            return _stopWriter;
        }

        void clearReadStop()
        {
            _stopReader = false;
        }

        void clearWriteStop()
        {
            _stopWriter = false;
        }

        void setMaxLatency(int maxLatency)
        {
            this->maxLatency = maxLatency;
        }

    private:
        T *_buffer;
        int size;
        int readc;
        int writec;
        int readable;
        int writable;
        int maxLatency;
        bool _stopReader;
        bool _stopWriter;
        std::mutex _readable_mtx;
        std::mutex _writable_mtx;
        std::condition_variable canReadVar;
        std::condition_variable canWriteVar;
    };
};