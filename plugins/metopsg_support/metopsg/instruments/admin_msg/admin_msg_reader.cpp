#include "admin_msg_reader.h"
#include "libs/bzlib/bzlib.h"
#include "logger.h"
#include <fstream>

namespace metopsg
{
    namespace admin_msg
    {
        AdminMsgReader::AdminMsgReader()
        {
            count = 0;
            message_out = new uint8_t[MAX_MSG_SIZE];
        }

        AdminMsgReader::~AdminMsgReader() { delete[] message_out; }

        void AdminMsgReader::work(ccsds::CCSDSPacket &packet)
        {
            unsigned int outsize = MAX_MSG_SIZE;

            int ret = BZ2_bzBuffToBuffDecompress((char *)message_out, &outsize, (char *)&packet.payload[39], packet.payload.size() - 39, 1, 1);
            if (ret != BZ_OK && ret != BZ_STREAM_END)
            {
                logger->error("Failed decomressing Bzip2 data! Error : " + std::to_string(ret));
                return;
            }

            std::string outputFileName = directory + "/" + std::to_string(packet.header.packet_sequence_count) + ".xml";
            logger->info("Writing message to " + outputFileName);
            std::ofstream outputMessageFile(outputFileName);
            outputMessageFile.write((char *)message_out, outsize);
            outputMessageFile.close();

            // TLE Parsing
            try
            {
                // std::string xml_str(message_out, &message_out[outsize]);

                // Parse
                doc.parse<0>((char *)message_out); // const char * to char * is needed...
                root_node = doc.first_node("multi-mission-administrative-message");

                // Convert to our usual TLE Registry stuff
                for (rapidxml::xml_node<> *sat_node = root_node->first_node("message"); sat_node; sat_node = sat_node->next_sibling())
                {
                    satdump::TLE tle;
                    tle.norad = std::stoi(sat_node->first_attribute("satellite-number")->value());
                    tle.name = sat_node->first_attribute("satellite")->value();

                    int linecnt = 0;
                    for (rapidxml::xml_node<> *sat_tle_node = sat_node->first_node("navigation"); sat_tle_node; sat_tle_node = sat_tle_node->next_sibling())
                    {
                        for (rapidxml::xml_node<> *tle_node = sat_tle_node->first_node("two-line-elements"); tle_node; tle_node = tle_node->next_sibling())
                        {
                            for (rapidxml::xml_node<> *line1_node = tle_node->first_node("line-1"); line1_node; line1_node = line1_node->next_sibling())
                            {
                                if (linecnt++ == 0)
                                    tle.line1 = line1_node->value();
                                else
                                    tle.line2 = line1_node->value();
                            }
                        }
                    }

                    // Done, save it
                    tles.push_back(tle);
                }
            }
            catch (std::exception &e)
            {
                logger->error("Error parsing Admin message : %s", e.what());
            }

            count++;
        }
    } // namespace admin_msg
} // namespace metopsg