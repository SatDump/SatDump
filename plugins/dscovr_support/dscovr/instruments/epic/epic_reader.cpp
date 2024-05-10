#include "epic_reader.h"
#include "logger.h"
#include "common/image/jpeg12_utils.h"
#include "common/image/io.h"

namespace dscovr
{
    namespace epic
    {
        EPICReader::EPICReader()
        {
        }

        EPICReader::~EPICReader()
        {
        }

        void EPICReader::work(uint8_t *pkt)
        {
            if (pkt[0] == 0x30 && pkt[1] == 0x37 && pkt[2] == 0x30 && pkt[3] == 0x37 && pkt[4] == 0x30)
            {
                if (wip_payload.size() > 140)
                {
                    int pos = 140;

                    /*  for (int i = 0; i < wip_file_vec.size() - 3; i++)
                      {
                          if (wip_file_vec[i + 0] == 0xFF && wip_file_vec[i + 1] == 0xD8 && wip_file_vec[i + 2] == 0xFF)
                          {
                              pos = i;
                              break;
                          }
                      }

                      logger->info(pos); */

                    std::string filename(&wip_payload[126], &wip_payload[126 + 8]);

                    auto img = image::decompress_jpeg12(wip_payload.data() + pos, wip_payload.size() - 140);
                    image::save_img(img, directory + "/" + filename);

                    img_c++;
                }

                wip_payload.clear();

                // if (wip_file_out.is_open())
                //{
                //     wip_file_out.close();
                // }

                // wip_file_out = std::ofstream("dscovrdata/" + std::to_string(fcnt++) + ".cpio", std::ios::binary);
                // wip_file_out.write((char *)pkt, 1080);
                wip_payload.insert(wip_payload.end(), pkt, pkt + 1080);
            }
            else
            {
                // wip_file_out.write((char *)pkt, 1080);
                wip_payload.insert(wip_payload.end(), pkt, pkt + 1080);
            }
        }
    }
}