#pragma once

#include "libserial/serial.h"
#include <mutex>

namespace satdump
{
    namespace exp_devs
    {
        class SatDumpDownconverter
        {
        private:
            serial_cpp::Serial uart;
            std::mutex mtx;

        private:
            bool sendCommand(std::string cmd, std::string &res)
            {
                std::scoped_lock l(mtx);

                uart.write(cmd + "\n");
                if (uart.waitReadable())
                {
                    res = uart.readline();
                    return false;
                }
                else
                {
                    return true;
                }
            }

        public:
            SatDumpDownconverter(std::string port) : uart(port, 9600, serial_cpp::Timeout::simpleTimeout(500)) { uart.flush(); }

            //////////////////////////////
            /////////// PLL
            //////////////////////////////

            double getPLLFreq()
            {
                try
                {
                    std::string r;
                    if (sendCommand("PLL:FREQ?", r))
                        return -1;
                    return std::stod(r);
                }
                catch (std::exception &e)
                {
                    return -1;
                }
            }

            bool setPLLFreq(double f)
            {
                std::string r;
                return sendCommand("PLL:FREQ " + std::to_string((uint64_t)f), r);
            }

            bool isPLLLocked()
            {
                try
                {
                    std::string r;
                    if (sendCommand("PLL:LOCK?", r))
                        return 0;
                    return std::stoi(r);
                }
                catch (std::exception &e)
                {
                    return 0;
                }
            }

            bool getPLLEnabled()
            {
                try
                {
                    std::string r;
                    if (sendCommand("PLL:STAT?", r))
                        return 0;
                    return std::stoi(r);
                }
                catch (std::exception &e)
                {
                    return 0;
                }
            }

            bool setPLLEnabled(bool e)
            {
                std::string r;
                return sendCommand("PLL:STAT " + std::to_string((int)e), r);
            }

            //////////////////////////////
            /////////// Attenator
            //////////////////////////////

            double getAttenuation()
            {
                try
                {
                    std::string r;
                    if (sendCommand("ATT:ATT?", r))
                        return -1;
                    return std::stod(r);
                }
                catch (std::exception &e)
                {
                    return -1;
                }
            }

            bool setAttenuation(double a)
            {
                std::string r;
                return sendCommand("ATT:ATT " + std::to_string(a), r);
            }

            //////////////////////////////
            /////////// Reference
            //////////////////////////////

            double getRefFreq()
            {
                try
                {
                    std::string r;
                    if (sendCommand("REF:FREQ?", r))
                        return -1;
                    return std::stod(r);
                }
                catch (std::exception &e)
                {
                    return -1;
                }
            }

            bool setRefFreq(double f)
            {
                std::string r;
                return sendCommand("REF:FREQ " + std::to_string((uint64_t)f), r);
            }

            bool isRefExternal()
            {
                try
                {
                    std::string r;
                    if (sendCommand("REF:EXT?", r))
                        return 0;
                    return std::stoi(r);
                }
                catch (std::exception &e)
                {
                    return 0;
                }
            }

            bool setRefExternal(bool e)
            {
                std::string r;
                return sendCommand("REF:EXT " + std::to_string((int)e), r);
            }

            //////////////////////////////
            /////////// Preamp
            //////////////////////////////

            bool getPreampEnabled()
            {
                try
                {
                    std::string r;
                    if (sendCommand("PRE:STAT?", r))
                        return 0;
                    return std::stoi(r);
                }
                catch (std::exception &e)
                {
                    return 0;
                }
            }

            bool setPreampEnabled(bool e)
            {
                std::string r;
                return sendCommand("PRE:STAT " + std::to_string((int)e), r);
            }

            //////////////////////////////
            /////////// Temperatures
            //////////////////////////////

            double getMCUTemp()
            {
                try
                {
                    std::string r;
                    if (sendCommand("TEMP:UC?", r))
                        return -1;
                    return std::stod(r);
                }
                catch (std::exception &e)
                {
                    return -1;
                }
            }

            double getRFTemp()
            {
                try
                {
                    std::string r;
                    if (sendCommand("TEMP:RF?", r))
                        return -1;
                    return std::stod(r);
                }
                catch (std::exception &e)
                {
                    return -1;
                }
            }

            double getVREGTemp()
            {
                try
                {
                    std::string r;
                    if (sendCommand("TEMP:VREG?", r))
                        return -1;
                    return std::stod(r);
                }
                catch (std::exception &e)
                {
                    return -1;
                }
            }
        };
    } // namespace exp_devs
} // namespace satdump