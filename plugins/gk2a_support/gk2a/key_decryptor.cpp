#include "key_decryptor.h"
#include <fstream>
#include <sstream>
#include "common/codings/crc/crc_generic.h"
#include "logger.h"

extern "C"
{
#include "libtom/tomcrypt_des.c"
}

namespace gk2a
{
    namespace lrit
    {
        std::string to_hex_str(uint8_t *array, int len)
        {
            std::string ss;
            for (int i = 0; i < len; i++)
            {
                const char hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                ss += hex_chars[(array[i] & 0xF0) >> 4];
                ss += hex_chars[(array[i] & 0x0F) >> 0];
            }
            return ss;
        }

        // Ported over from Sam's xrit-rx
        bool decrypt_key_file(std::string encrypted, std::string mac_address, std::string decrypted)
        {
            uint8_t km_bytes[550];
            std::ifstream fin(encrypted, std::ios::binary);
            fin.read((char *)km_bytes, 550);
            fin.close();

            int header_len = 8;
            int data_len = 540;
            int crc_len = 2;

            uint8_t *data = km_bytes + header_len;
            uint8_t *crc = km_bytes + header_len + data_len;

            codings::crc::GenericCRC crcc(16, 0x1021, 0xFFFF, 0x0000, false, false);

            uint16_t crc_l = crcc.compute(km_bytes, header_len + data_len);
            uint16_t local_crc = crc[0] << 8 | crc[1];

            if (crc_l != local_crc)
            {
                logger->info("CRC Invalid!");
                return true;
            }

            logger->info("CRC is valid!");

            // Time header
            auto km_hex = to_hex_str(km_bytes, 8);
            std::string appYear = km_hex.substr(0, 4);
            std::string appMonth = km_hex.substr(4, 2);
            std::string appDay = km_hex.substr(6, 2);
            std::string appHour = km_hex.substr(8, 2);
            std::string appMin = km_hex.substr(10, 2);
            std::string appSecS = km_hex.substr(12, 4);
            int appSec = round(std::stoi(appSecS) / 1000); // kmHeaderHex[12:16]

            logger->info("Application Time header: {:s} ({:s}/{:s}/{:s} {:s}:{:s}:{:d})", km_hex.c_str(), appDay.c_str(), appMonth.c_str(), appYear.c_str(), appHour.c_str(), appMin.c_str(), appSec);

            // Encrypted keys
            std::vector<uint16_t> indexes;
            std::vector<std::vector<uint8_t>> encKeys;

            for (int i = 0; i < 30; i++) // 30 keys total
            {
                int offset = i * 18;                                                            // 18 bytes per index/key pair
                indexes.push_back(data[offset] << 8 | data[offset + 1]);                        // Bytes 0-1: Key index
                encKeys.push_back(std::vector<uint8_t>(&data[offset + 2], &data[offset + 18])); // Bytes 2-17: Encrypted key
                logger->info("({:s}) = {:s}", to_hex_str((uint8_t *)&indexes[i], 1).c_str(), to_hex_str(encKeys[i].data(), 16).c_str());
            }

            // Decrypt
            logger->info("\nDecrypting...");

            uint64_t mac_bin = 0;
            uint64_t mac_bin2 = std::stoul(mac_address, nullptr, 16) << 16; //;

            for (int i = 0; i < 8; i++)
            {
                uint8_t v = (mac_bin2 >> (i)*8) & 0xFF;
                mac_bin = mac_bin << 8 | v;
            }

            logger->info("Key is " + to_hex_str((uint8_t *)&mac_bin, 8));

            symmetric_key sk;
            des_setup((unsigned char *)&mac_bin, 8, 0, &sk);

            std::vector<std::vector<uint8_t>> decKeys;
            for (int i = 0; i < 30; i++)
            {
                decKeys.push_back(std::vector<uint8_t>(8));
                des_ecb_decrypt(encKeys[i].data(), decKeys[i].data(), &sk);
                des_ecb_decrypt(encKeys[i].data(), decKeys[i].data(), &sk);
                //  encKeys[i].erase(encKeys.begin() + )
                logger->info("({:s}) = {:s}", to_hex_str((uint8_t *)&indexes[i], 1).c_str(), to_hex_str(decKeys[i].data(), 8).c_str());
            }

            std::ofstream file_decrypted(decrypted, std::ios::binary);
            file_decrypted.put(0x00);
            file_decrypted.put(0x1E);
            for (int i = 0; i < 30; i++)
            {
                indexes[i] = indexes[i] >> 8 | (indexes[i] & 0xFF) << 8;
                file_decrypted.write((char *)&indexes[i], 2);
                file_decrypted.write((char *)decKeys[i].data(), 8);
            }

            return false;
        }
    }
}