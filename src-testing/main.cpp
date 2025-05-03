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
#include "products2/image/product_expression.h"
#include "products2/image_product.h"
#include <exception>

int main(int argc, char *argv[])
{
    initLogger();

    satdump::products::ImageProduct pro;
    pro.load("/home/alan/Downloads/SatDump_NEWPRODS/metop_test/AVHRR/product.cbor");

    bool v = satdump::products::check_expression_product_composite(&pro, "cha, ch2, ch1");
    logger->critical(v);
}
