#include "simulator_handler.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "dsp/channel_model/channel_model_simple.h"
#include "imgui/imgui.h"
#include "init.h"
#include "logger.h"
#include "simulator/worker.h"
#include "utils/event_bus.h"
#include <memory>

#include "dsp/channel_model/model/antenna/dipole.h"
#include "dsp/channel_model/model/antenna/parabolic_reflector.h"
#include "utils/time.h"

namespace satdump
{
    namespace ominous
    {
        SimulatorHandler::SimulatorHandler()
        {
            handler_tree_icon = u8"\uf471";

            core.pkt_bus = std::make_shared<EventBus>();

            /*core.pkt_bus->register_handler<PushSpacePacketEvent>(
                [](PushSpacePacketEvent e)
                {
                    //
                    logger->trace("Got PKT VCID %d APID %d SIZE %d", e.vcid, e.pkt.header.apid, e.pkt.payload.size() + 6);
                });*/

            int norad = 25338;
            core.tracker = std::make_shared<SatelliteTracker>(satdump::db_tle->get_from_norad_time(norad, time(0)).value());

            // LRPT Modulator
            lrpt_modulator = std::make_shared<LRPTModulator>(core);

            lrpt_throttle = std::make_shared<ndsp::ThrottleBlock<complex_t>>();
            lrpt_throttle->set_cfg("samplerate", 80e3 * 4);
            lrpt_throttle->set_input(lrpt_modulator->output_stream, 0);

            lrpt_channel_model = std::make_shared<ndsp::ChannelModelSimpleBlock>();
            lrpt_channel_model->set_cfg("samplerate", 80e3 * 4);
            lrpt_channel_model->set_cfg("freq_shift", 2e3);
            lrpt_channel_model->link(lrpt_throttle.get(), 0, 0, 4);

            lrpt_nngsink = std::make_shared<ndsp::NNGIQSinkBlock>();
            lrpt_nngsink->link(lrpt_channel_model.get(), 0, 0, 4);

            lrpt_throttle->start();
            lrpt_channel_model->start();
            lrpt_nngsink->start();

            // HRPT Modulator
            hrpt_modulator = std::make_shared<HRPTModulator>(core);

            hrpt_throttle = std::make_shared<ndsp::ThrottleBlock<complex_t>>();
            hrpt_throttle->set_cfg("samplerate", 2.5e6 * 4);
            hrpt_throttle->set_input(hrpt_modulator->output_stream, 0);

            hrpt_channel_model = std::make_shared<ndsp::ChannelModelSimpleBlock>();
            hrpt_channel_model->set_cfg("samplerate", 2.5e6 * 4);
            hrpt_channel_model->set_cfg("freq_shift", 2e3);
            hrpt_channel_model->link(hrpt_throttle.get(), 0, 0, 4);

            hrpt_nngsink = std::make_shared<ndsp::NNGIQSinkBlock>();
            hrpt_nngsink->link(hrpt_channel_model.get(), 0, 0, 4);
            hrpt_nngsink->set_cfg("port", 8899);

            hrpt_throttle->start();
            hrpt_channel_model->start();
            hrpt_nngsink->start();

            // UDP to real hardware
            lrpt_udp_sink = std::make_shared<LRPTUdpSink>(core);

            {
                lrpt_channel_model->set_cfg("signal_level", lrpt_signal_level);
                lrpt_channel_model->set_cfg("noise_level", lrpt_noise_level);
                lrpt_channel_model->set_cfg("freq_shift", lrpt_freq_shift);

                hrpt_channel_model->set_cfg("signal_level", hrpt_signal_level);
                hrpt_channel_model->set_cfg("noise_level", hrpt_noise_level);
                hrpt_channel_model->set_cfg("freq_shift", hrpt_freq_shift);

                lrpt_model = std::make_shared<ChannelModelLEO>(false,                                                                                       //
                                                               FREE_SPACE_PATH_LOSS,                                                                        //
                                                               IMPAIRMENT_NONE,                                                                             //
                                                               DOPPLER_SHIFT,                                                                               //
                                                               IMPAIRMENT_NONE,                                                                             //
                                                               IMPAIRMENT_NONE,                                                                             //
                                                               true,                                                                                        //
                                                               7.5,                                                                                         //
                                                               20,                                                                                          //
                                                               90,                                                                                          //
                                                               std::make_shared<antenna::DipoleAntenna>(antenna::GenericAntenna::DIPOLE, 137.5e6, RHCP, 0), //
                                                               std::make_shared<antenna::DipoleAntenna>(antenna::GenericAntenna::DIPOLE, 137.5e6, RHCP, 0), //
                                                               137.5e6,                                                                                     //
                                                               37,                                                                                          //
                                                               1,                                                                                           //
                                                               210,                                                                                         //
                                                               320e3);

                hrpt_model = std::make_shared<ChannelModelLEO>(false,                                                                                                                         //
                                                               FREE_SPACE_PATH_LOSS,                                                                                                          //
                                                               IMPAIRMENT_NONE,                                                                                                               //
                                                               DOPPLER_SHIFT,                                                                                                                 //
                                                               IMPAIRMENT_NONE,                                                                                                               //
                                                               IMPAIRMENT_NONE,                                                                                                               //
                                                               true,                                                                                                                          //
                                                               7.5,                                                                                                                           //
                                                               20,                                                                                                                            //
                                                               90,                                                                                                                            //
                                                               std::make_shared<antenna::ParabolicReflectorAntenna>(antenna::GenericAntenna::PARABOLIC_REFLECTOR, 1700e6, RHCP, 0, 0.11, 55), //
                                                               std::make_shared<antenna::ParabolicReflectorAntenna>(antenna::GenericAntenna::PARABOLIC_REFLECTOR, 1700e6, RHCP, 0, 0.8, 55),  //
                                                               1700e6,                                                                                                                        //
                                                               30,                                                                                                                            //
                                                               1,                                                                                                                             //
                                                               50,                                                                                                                            //
                                                               2.5e6);
            }

            // NavAtt Instrument
            // navatt_simulator = std::make_shared<NavAttSimulator>(core);

            // CSR Instrument
            csr_simulator = std::make_shared<CSRSimulator>(core);

            // CSR LRPT Encoder
            csr_lrpt_encoder = std::make_shared<CSRLRPTSimulator>(core);

            // LORIS Instrument
            loris_simulator = std::make_shared<LORISSimulator>(core);

            // LORIS comp encoer
            loris_proc_encoder = std::make_shared<LORISProcessor>(core);

            // CHEAP Instrument
            // cheap_simulator = std::make_shared<CHEAPSimulator>(core);

            // EAGLE Instrument
            // eagle_simulator = std::make_shared<EAGLESimulator>(core);
        }

        SimulatorHandler::~SimulatorHandler()
        {
            lrpt_modulator.reset();
            hrpt_modulator.reset();

            lrpt_throttle->stop();
            lrpt_channel_model->stop();
            lrpt_nngsink->stop();

            hrpt_throttle->stop();
            hrpt_channel_model->stop();
            hrpt_nngsink->stop();
        }

        void SimulatorHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Control"))
            {
                ImGui::SeparatorText("LRPT");
                if (ImGui::Button("LRPT Mode"))
                    lrpt_modulator->set_lrpt_mode(true);
                if (ImGui::Button("DSB Mode"))
                    lrpt_modulator->set_lrpt_mode(false);

                ImGui::InputDouble("Signal Level", &lrpt_signal_level);
                ImGui::InputDouble("Noise Level", &lrpt_noise_level);
                ImGui::InputDouble("Freq Shift", &lrpt_freq_shift);
                if (ImGui::Button("Set") || lrpt_use_model)
                {
                    lrpt_channel_model->set_cfg("signal_level", lrpt_signal_level);
                    lrpt_channel_model->set_cfg("noise_level", lrpt_noise_level);
                    lrpt_channel_model->set_cfg("freq_shift", lrpt_freq_shift);
                }
                ImGui::Checkbox("Use Model##lrpt", &lrpt_use_model);

                ImGui::SeparatorText("HRPT");
                if (ImGui::Button("QPSK Mode"))
                    hrpt_modulator->set_qpsk_mode(true);
                if (ImGui::Button("BPSK Mode"))
                    hrpt_modulator->set_qpsk_mode(false);

                ImGui::InputDouble("Signal Level##hrpt", &hrpt_signal_level);
                ImGui::InputDouble("Noise Level##hrpt", &hrpt_noise_level);
                ImGui::InputDouble("Freq Shift##hrpt", &hrpt_freq_shift);
                if (ImGui::Button("Set##hrpt") || hrpt_use_model)
                {
                    hrpt_channel_model->set_cfg("signal_level", hrpt_signal_level);
                    hrpt_channel_model->set_cfg("noise_level", hrpt_noise_level);
                    hrpt_channel_model->set_cfg("freq_shift", hrpt_freq_shift);
                }
                ImGui::Checkbox("Use Model##hrpt", &hrpt_use_model);

                ImGui::SeparatorText("LRPT Modulator Filters");
                ImGui::InputInt("VCID", &filter_vcid);
                ImGui::InputInt("APID", &filter_apid);
                if (ImGui::Button("Add"))
                    lrpt_modulator->add_pkt_filter(filter_vcid, filter_apid);
                if (ImGui::Button("Del"))
                    lrpt_modulator->del_pkt_filter(filter_vcid, filter_apid);
            }

            if (ImGui::CollapsingHeader("QTH"))
            {
                ImGui::InputDouble("Latitude", &qth_lat);
                ImGui::InputDouble("Longitude", &qth_lon);
                ImGui::InputDouble("Altitude", &qth_alt);
            }

            auto pos = core.tracker->get_observed_position(getTime(), qth_lat, qth_lon, qth_alt);

            if (lrpt_use_model)
            {
                lrpt_model->set_sat_info(pos.range, pos.elevation * RAD_TO_DEG, pos.range_rate);
                lrpt_model->update();

                if (pos.elevation > 0)
                    lrpt_signal_level = lrpt_noise_level + lrpt_model->get_link_margin();
                else
                    lrpt_signal_level = lrpt_noise_level - 100;
                lrpt_freq_shift = lrpt_model->get_doppler_freq();
            }

            if (hrpt_use_model)
            {
                hrpt_model->set_sat_info(pos.range, pos.elevation * RAD_TO_DEG, pos.range_rate);
                hrpt_model->update();

                if (pos.elevation > 0)
                    hrpt_signal_level = hrpt_noise_level + hrpt_model->get_link_margin();
                else
                    hrpt_signal_level = hrpt_noise_level - 100;
                hrpt_freq_shift = hrpt_model->get_doppler_freq();
            }

            logger->trace("%f %f %f => %f / %f", pos.range, pos.elevation * RAD_TO_DEG, pos.range_rate, lrpt_model->get_link_margin(), hrpt_model->get_link_margin());
        }

        void SimulatorHandler::drawMenuBar() {}

        void SimulatorHandler::drawContents(ImVec2 win_size)
        {
            ImVec2 window_size = win_size;

            ImGui::Text("Stuff here!");
        }
    } // namespace ominous
} // namespace satdump
