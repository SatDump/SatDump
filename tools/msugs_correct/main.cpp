/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include "image/image.h"
#include "common/projection/projs/tps_transform.h"

image::Image<uint8_t> correct_msugs_channel(std::vector<satdump::projection::GCP> &gcps1, std::vector<satdump::projection::GCP> &gcps2, image::Image<uint8_t> image)
{
    image::Image<uint8_t> half1 = image, half2 = image;
    half1.crop(0, 6000);
    half2.crop(6000, 12000);

    image::Image<uint8_t> image_rgb(11136, 11136, 1);

    satdump::projection::TPSTransform tps_tf2(gcps2);

    for (int x = 0; x < 11136; x++)
    {
        for (int y = 0; y < 11136; y++)
        {
            double xx, yy;
            tps_tf2.forward(x, y, xx, yy);
            xx -= 6000;

            if (xx >= half2.width())
                continue;
            if (xx < 0)
                continue;

            if (yy >= half2.height())
                continue;
            if (yy < 0)
                continue;

            image_rgb.channel(0)[y * 11136 + x] = half2[int(yy) * half2.width() + int(xx)];
        }
    }

    satdump::projection::TPSTransform tps_tf1(gcps1);

    for (int x = 0; x < 11136; x++)
    {
        for (int y = 0; y < 11136; y++)
        {
            double xx, yy;
            tps_tf1.forward(x, y, xx, yy);

            if (xx >= half1.width())
                continue;
            if (xx < 0)
                continue;

            if (yy >= half1.height())
                continue;
            if (yy < 0)
                continue;

            image_rgb.channel(0)[y * 11136 + x] = half1[int(yy) * half1.width() + int(xx)];
        }
    }

    return image_rgb;
}

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    if (argc < 5)
    {
        logger->error("Not enough arguments");
        return 1;
    }

    std::vector<satdump::projection::GCP> ch3_gcps1, ch3_gcps2;
    std::vector<satdump::projection::GCP> ch2_gcps1, ch2_gcps2;
    std::vector<satdump::projection::GCP> ch1_gcps1, ch1_gcps2;

    /////////////////////////////////////////////////////////////////////////////// CH3
    // 1
    ch3_gcps1.push_back({2712, 9458, 2282, 9356}); // South America bottom
    ch3_gcps1.push_back({1359, 4332, 778, 4322});  // South America top
    ch3_gcps1.push_back({5984, 499, 5179, 372});   // Cloud north pole
    ch3_gcps1.push_back({2519, 1663, 1805, 1701}); // Great lakes
    ch3_gcps1.push_back({5986, 3639, 5305, 3345}); // Africa tip
    ch3_gcps1.push_back({2220, 8642, 1771, 8543}); // SA tip in lake
    ch3_gcps1.push_back({3461, 2395, 2767, 2285}); // Cloud over altantic
    ch3_gcps1.push_back({5194, 1947, 4457, 1750}); // Cloud over altantic 2
    ch3_gcps1.push_back({2498, 3915, 1883, 3809}); // Cloud over altantic 3
    ch3_gcps1.push_back({3439, 4305, 2826, 4128}); // Cloud over altantic 4
    ch3_gcps1.push_back({3046, 6012, 2502, 5827}); // SA border
    ch3_gcps1.push_back({5969, 9860, 5537, 9586}); // South pole cloud
    ch3_gcps1.push_back({4405, 8114, 3920, 7860}); // Cloud over altantic 5
    ch3_gcps1.push_back({2523, 2615, 1851, 2567}); // Cloud over altantic 6
    ch3_gcps1.push_back({1343, 3348, 721, 3397});  // Cuba
    ch3_gcps1.push_back({1504, 3794, 897, 3787});  // Cuba Spike
    ch3_gcps1.push_back({3527, 1740, 2804, 1659}); // Cloud
    ch3_gcps1.push_back({4180, 667, 3401, 644});   // Cloud
    ch3_gcps1.push_back({2451, 2401, 1770, 2377}); // Cloud
    ch3_gcps1.push_back({5992, 1365, 5220, 1166}); // Cloud
    ch3_gcps1.push_back({5997, 6652, 5434, 6319}); // Cloud
    ch3_gcps1.push_back({5033, 1465, 4279, 1304}); // Cloud
    ch3_gcps1.push_back({5143, 629, 4352, 532});   // Cloud

    // 2
    ch3_gcps2.push_back({7478, 650, 6603, 452});     // Top sweden
    ch3_gcps2.push_back({6041, 3631, 5305, 3346});   // Africa spike
    ch3_gcps2.push_back({10013, 2571, 9173, 2182});  // Greece island
    ch3_gcps2.push_back({6456, 10110, 5977, 9830});  // South pole cloud
    ch3_gcps2.push_back({7605, 2086, 6780, 1780});   // Spain spike
    ch3_gcps2.push_back({7004, 1251, 6157, 1020});   // UK spike
    ch3_gcps2.push_back({8480, 1796, 7633, 1474});   // Italy Cloud
    ch3_gcps2.push_back({10050, 2003, 9191, 1664});  // Russia crochet
    ch3_gcps2.push_back({9561, 1821, 8701, 1484});   // Russia spike
    ch3_gcps2.push_back({6014, 486, 5152, 372});     // North pole spike
    ch3_gcps2.push_back({11430, 4829, 10648, 4345}); // Africa stick
    ch3_gcps2.push_back({7144, 6042, 6481, 5658});   // Atlantic cloud 1
    ch3_gcps2.push_back({8654, 10083, 8160, 9697});  // North pole island
    ch3_gcps2.push_back({9298, 1275, 8425, 1003});   // Russia cloud 1
    ch3_gcps2.push_back({6501, 7705, 5920, 7360});   // Atlantic cloud 2
    ch3_gcps2.push_back({6037, 8760, 5506, 8462});   // Atlantic cloud 3
    ch3_gcps2.push_back({8652, 6505, 7985, 6050});   // Atlantic cloud 4
    ch3_gcps2.push_back({10081, 8632, 9499, 8145});  // Africa spike 2
    ch3_gcps2.push_back({9356, 3830, 8569, 3390});   // Africa cloud 1
    ch3_gcps2.push_back({8924, 7513, 8300, 7047});   // Africa border 1
    ch3_gcps2.push_back({7927, 3169, 7138, 2806});   // Africa cloud 2
    ch3_gcps2.push_back({10436, 6735, 9754, 6215});  // Africa border 2
    ch3_gcps2.push_back({11121, 3576, 10298, 3148}); // Russian cloud 1
    ch3_gcps2.push_back({10649, 8508, 10055, 8013}); // African border 1
    ch3_gcps2.push_back({11259, 7175, 10586, 6653}); // Cloud 1
    ch3_gcps2.push_back({7797, 8215, 7219, 7807});   // Cloud 2
    ch3_gcps2.push_back({9317, 2932, 8498, 2530});   // Africa border 1
    ch3_gcps2.push_back({6008, 4219, 5295, 3925});   // Cloud 3
    ch3_gcps2.push_back({10641, 5016, 9881, 4517});  // Cloud 4
    ch3_gcps2.push_back({10116, 9325, 9573, 8870});  // Cloud 5
    ch3_gcps2.push_back({7815, 9576, 7301, 9206});   // Cloud 6
    ch3_gcps2.push_back({10811, 4193, 10016, 3722}); // Africa border
    ch3_gcps2.push_back({8500, 2311, 7671, 1963});   // Africa border
    ch3_gcps2.push_back({6471, 411, 5602, 284});     // Cloud

    /////////////////////////////////////////////////////////////////////////////// CH2
    // 1
    ch2_gcps1.push_back({4186, 718, 3413, 679});     // North Pole
    ch2_gcps1.push_back({3791, 6355, 3253, 6123});   // South America thing
    ch2_gcps1.push_back({5896, 4286, 5245, 3968});   // South America thing 2
    ch2_gcps1.push_back({2711, 9459, 2282, 9355});   // South America thing 3
    ch2_gcps1.push_back({5987, 501, 5187, 361});     // North pole spike
    ch2_gcps1.push_back({5982, 3655, 5304, 3344});   // Africa spike
    ch2_gcps1.push_back({5993, 6288, 5421, 5943});   // Cloud
    ch2_gcps1.push_back({5991, 10285, 5583, 10018}); // Cloud
    ch2_gcps1.push_back({1490, 3054, 852, 3106});    // Florida
    ch2_gcps1.push_back({3906, 7626, 3411, 7388});   // Cloud
    ch2_gcps1.push_back({5988, 7856, 5479, 7513});   // Cloud
    ch2_gcps1.push_back({3484, 2920, 2813, 2780});   // Cloud
    ch2_gcps1.push_back({1331, 4323, 746, 4323});    // Cloud
    ch2_gcps1.push_back({1799, 3743, 1185, 3712});   // Cloud
    ch2_gcps1.push_back({5137, 1393, 4383, 1219});   // Cloud
    ch2_gcps1.push_back({2396, 1994, 1698, 2011});   // Cloud
    ch2_gcps1.push_back({5953, 2895, 5245, 2613});   // Cloud
    ch2_gcps1.push_back({5192, 8860, 4731, 8575});   // Cloud
    ch2_gcps1.push_back({1959, 8810, 1516, 8747});   // SA
    ch2_gcps1.push_back({4196, 1575, 3459, 1450});   // Cloud
    ch2_gcps1.push_back({5261, 1978, 4528, 1769});   // Cloud
    ch2_gcps1.push_back({2383, 2882, 1724, 2836});   // Cloud
    ch2_gcps1.push_back({2141, 7493, 1657, 7375});   // Cloud over SA
    ch2_gcps1.push_back({997, 6513, 498, 6511});     // Cloud over SA
    ch2_gcps1.push_back({3557, 941, 2800, 942});     // NA Lake
    ch2_gcps1.push_back({3669, 1315, 2926, 1253});   // Cloud
    ch2_gcps1.push_back({4294, 9629, 3867, 9415});   // Cloud

    // 2
    ch2_gcps2.push_back({7577, 2098, 6780, 1780});   // Spain spike
    ch2_gcps2.push_back({9981, 2574, 9171, 2182});   // Greece island spike
    ch2_gcps2.push_back({7644, 500, 6791, 315});     // Norway spike
    ch2_gcps2.push_back({8252, 997, 7409, 737});     // Norway spike 2
    ch2_gcps2.push_back({9269, 9324, 8761, 8874});   // Africa spike 1
    ch2_gcps2.push_back({6021, 497, 5187, 361});     // North pole spike
    ch2_gcps2.push_back({6014, 3650, 5304, 3344});   // Africa spike
    ch2_gcps2.push_back({6026, 6284, 5421, 5943});   // Cloud
    ch2_gcps2.push_back({6024, 10281, 5583, 10018}); // Cloud
    ch2_gcps2.push_back({7044, 798, 6207, 585});     // UK
    ch2_gcps2.push_back({7574, 1150, 6743, 888});    // FR Top
    ch2_gcps2.push_back({8768, 7018, 8152, 6548});   // FR Top
    ch2_gcps2.push_back({9287, 2936, 8498, 2530});   // Africa
    ch2_gcps2.push_back({11396, 4824, 10649, 4346}); // Africa stick
    ch2_gcps2.push_back({6955, 8048, 6407, 7666});   // Clouds
    ch2_gcps2.push_back({6021, 7852, 5479, 7513});   // Cloud
    ch2_gcps2.push_back({10733, 6186, 10055, 5668}); // Africa lake
    ch2_gcps2.push_back({10313, 3929, 9545, 3468});  // Africa cloud
    ch2_gcps2.push_back({10617, 8508, 10056, 8012}); // Asia border
    ch2_gcps2.push_back({9977, 7910, 9390, 7403});   // Africa thing
    ch2_gcps2.push_back({10494, 2490, 9676, 2124});  // Russia thing
    ch2_gcps2.push_back({9443, 1444, 8603, 1144});   // Russia thing 2
    ch2_gcps2.push_back({8610, 10082, 8146, 9689});  // Russia thing 2
    ch2_gcps2.push_back({7657, 10501, 7217, 10173}); // North pole clouds
    ch2_gcps2.push_back({9306, 1922, 8482, 1574});   // Russia thing
    ch2_gcps2.push_back({8719, 1508, 7888, 1191});   // Russia thing
    ch2_gcps2.push_back({7079, 9112, 6576, 8746});   // Cloud
    ch2_gcps2.push_back({8987, 4248, 8250, 3803});   // Cloud
    ch2_gcps2.push_back({7118, 10102, 6660, 9770});  // Cloud

    /////////////////////////////////////////////////////////////////////////////// CH1
    // 1
    ch1_gcps1.push_back({5991, 3577, 5317, 3248});   // Africa spike
    ch1_gcps1.push_back({3003, 8567, 2556, 8395});   // SA
    ch1_gcps1.push_back({3879, 6325, 3345, 6073});   // Cloud
    ch1_gcps1.push_back({4811, 681, 4038, 576});     // Ice NP
    ch1_gcps1.push_back({1502, 3730, 902, 3732});    // Cuba
    ch1_gcps1.push_back({6000, 9854, 5580, 9539});   // Cloud
    ch1_gcps1.push_back({6001, 7497, 5484, 7127});   // Cloud
    ch1_gcps1.push_back({5994, 1852, 5251, 1591});   // Cloud
    ch1_gcps1.push_back({3040, 4504, 2447, 4335});   // Cloud
    ch1_gcps1.push_back({5990, 552, 5197, 387});     // Cloud
    ch1_gcps1.push_back({5248, 1461, 4502, 1259});   // Cloud
    ch1_gcps1.push_back({2887, 9662, 2473, 9542});   // Cloud
    ch1_gcps1.push_back({1604, 2890, 967, 2935});    // Florida
    ch1_gcps1.push_back({3210, 2244, 2522, 2147});   // Cloud
    ch1_gcps1.push_back({1136, 6804, 654, 6783});    // SA
    ch1_gcps1.push_back({2516, 2972, 1868, 2900});   // Cloud
    ch1_gcps1.push_back({4589, 9430, 4161, 9175});   // Cloud
    ch1_gcps1.push_back({4793, 3151, 4120, 2899});   // Cloud
    ch1_gcps1.push_back({3358, 1143, 2619, 1122});   // Cloud
    ch1_gcps1.push_back({2422, 5158, 1863, 5023});   // Cloud
    ch1_gcps1.push_back({892, 5871, 384, 5888});     // Cloud
    ch1_gcps1.push_back({998, 4534, 439, 4571});     // Cloud
    ch1_gcps1.push_back({5640, 10684, 5255, 10436}); // Cloud

    // 2
    ch1_gcps2.push_back({7553, 2114, 6780, 1780});   // Spain spike
    ch1_gcps2.push_back({9961, 2577, 9172, 2181});   // Greece
    ch1_gcps2.push_back({8055, 1050, 7237, 771});    // Idk
    ch1_gcps2.push_back({6895, 793, 6082, 569});     // Idk
    ch1_gcps2.push_back({6007, 3570, 5317, 3248});   // Africa spike
    ch1_gcps2.push_back({8839, 8412, 8310, 7940});   // Africa spike 2
    ch1_gcps2.push_back({6016, 9847, 5580, 9539});   // Cloud
    ch1_gcps2.push_back({6017, 7491, 5484, 7127});   // Cloud
    ch1_gcps2.push_back({6965, 1288, 6169, 1019});   // UK
    ch1_gcps2.push_back({6009, 1846, 5251, 1591});   // Cloud
    ch1_gcps2.push_back({6006, 546, 5197, 387});     // Cloud
    ch1_gcps2.push_back({9408, 3806, 8670, 3357});   // Cloud
    ch1_gcps2.push_back({9266, 2946, 8498, 2530});   // Africa
    ch1_gcps2.push_back({10977, 6997, 10353, 6474}); // Cloud
    ch1_gcps2.push_back({11377, 4822, 10649, 4346}); // Cloud
    ch1_gcps2.push_back({9112, 9392, 8631, 8937});   // Cloud
    ch1_gcps2.push_back({10525, 5598, 9843, 5085});  // Cloud
    ch1_gcps2.push_back({7872, 6303, 7258, 5855});   // Cloud
    ch1_gcps2.push_back({9513, 1869, 8707, 1520});   // Russia spike
    ch1_gcps2.push_back({11022, 3515, 10250, 3089}); // Fire
    ch1_gcps2.push_back({10768, 4211, 10025, 3737}); // Africa
    ch1_gcps2.push_back({7815, 9588, 7351, 9184});   // Cloud
    ch1_gcps2.push_back({9043, 1193, 8220, 907});    // Cloud
    ch1_gcps2.push_back({7580, 536, 6754, 332});     // Cloud
    ch1_gcps2.push_back({8697, 1515, 7888, 1192});   // Cloud
    ch1_gcps2.push_back({8665, 2116, 7876, 1753});   // Italy
    ch1_gcps2.push_back({7534, 995, 6720, 732});     // Cloud
    ch1_gcps2.push_back({7557, 10646, 7146, 10315}); // Cloud

    logger->debug("Loading images...");
    image::Image<uint8_t> image_1, image_2, image_3, image_rgb;
    image_1.load_png(argv[1]);
    image_2.load_png(argv[2]);
    image_3.load_png(argv[3]);
    image_rgb.init(11136, 11136, 3);

    logger->debug("Denoise...");
    image_1.median_blur();
    image_2.median_blur();
    image_3.median_blur();

    logger->debug("Correct 1...");
    image::Image<uint8_t> img1 = correct_msugs_channel(ch1_gcps1, ch1_gcps2, image_1);
    logger->debug("Correct 2...");
    image::Image<uint8_t> img2 = correct_msugs_channel(ch2_gcps1, ch2_gcps2, image_2);
    logger->debug("Correct 3...");
    image::Image<uint8_t> img3 = correct_msugs_channel(ch3_gcps1, ch3_gcps2, image_3);

    // img1.save_png("chch1.png");
    // img2.save_png("chch2.png");
    // img3.save_png("chch3.png");

    logger->debug("Compose...");
    image_rgb.draw_image(0, img3);
    image_rgb.draw_image(1, img2);
    image_rgb.draw_image(2, img1);

    logger->debug("Save...");
    image_rgb.save_png(argv[4]);
}
