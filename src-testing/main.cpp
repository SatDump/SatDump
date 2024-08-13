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

#include "common/dsp/complex.h"
#include "common/ndsp/block.h"
#include <unistd.h>

#include "common/ndsp/filter/fir.h"
#include "common/ndsp/pll/costas_loop.h"
#include "common/ndsp/clock/clock_recovery_mm.h"

#include "common/dsp/filter/firdes.h"

#include <fstream>

int main(int argc, char *argv[])
{
    initLogger();

    std::shared_ptr<NaFiFo> fifo1 = std::make_shared<NaFiFo>();
    ndsp::buf::init_nafifo_stdbuf<complex_t>(fifo1, 2, 8192);

    ndsp::FIRFilter<complex_t> rrc_filter;
    ndsp::CostasLoop costas;
    ndsp::ClockRecoveryMM<complex_t> clock_reco;

    rrc_filter.d_taps = dsp::firdes::root_raised_cosine(8.0, 6e6, 2333333, 0.35, 31);

    costas.d_order = 4;
    costas.d_loop_bw = 0.005;
    costas.d_freq_limit = 1;

    clock_reco.d_omega = 6e6 / 2333333;
    clock_reco.d_omega_gain = pow(8.7e-3, 2) / 4.0;
    clock_reco.d_mu = 0.5f;
    clock_reco.d_mu_gain = 8.7e-3;
    clock_reco.d_omega_relative_limit = 0.005f;

    rrc_filter.set_input(0, fifo1);
    costas.set_input(0, rrc_filter.get_output(0));
    clock_reco.set_input(0, costas.get_output(0));

    rrc_filter.start();
    costas.start();
    clock_reco.start();

    bool should_run = true;

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_ou(argv[2], std::ios::binary);

    std::thread thread_tx([fifo1, &should_run, &data_in]()
                          {
        while(should_run && !data_in.eof()) {
            data_in.read((char*) ((ndsp::buf::StdBuf<complex_t>*) fifo1->read_buf())->dat, 4096 * sizeof(complex_t));
            ((ndsp::buf::StdBuf<complex_t>*) fifo1->read_buf())->cnt = 4096;
            fifo1->write();
        } });

    std::thread thread_rx([&clock_reco, &data_ou]()
                          {
                              while (!clock_reco.outputs[0]->read())
                              {
                                 auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)clock_reco.outputs[0]->read_buf();
                                 // for (int i = 0; i < wbuf->cnt; i++)
                                 //     printf("%f, ", wbuf->dat[i]);
                                 // printf("\n");
                                  data_ou.write((char*)wbuf->dat, wbuf->cnt * sizeof(complex_t));
                                  
                                  clock_reco.outputs[0]->flush();
                              } });

    // sleep(2);
    while (!data_in.eof())
    {
        sleep(1);
        printf("%llu \n", data_in.tellg());
    }

    should_run = false;
    fifo1->stop();
    if (thread_tx.joinable())
        thread_tx.join();

    logger->info("Stopping blocks!");

    rrc_filter.stop();
    costas.stop();
    clock_reco.stop();

    logger->info("Blocks stopped!");

    if (thread_rx.joinable())
        thread_rx.join();
}