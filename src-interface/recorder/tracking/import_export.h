#pragma once
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/tracking/obj_tracker/object_tracker.h"
#include "common/tracking/scheduler/scheduler.h"
#include "common/widgets/notated_num.h"
#include "common/widgets/timed_message.h"
#include "imgui/dialogs/widget.h"

namespace satdump
{
    class TrackingImportExport
    {
    private:
        // Export
        FileSelectWidget output_directory = FileSelectWidget("exportoutputdirectory", "Output Directory", true);
        widgets::NotatedNum<uint64_t> initial_frequency = widgets::NotatedNum<uint64_t>("Inital Frequency", 100e6, "Hz");
        widgets::TimedMessage export_message, import_message;
        std::shared_ptr<dsp::DSPSampleSource> source_obj;
        std::vector<std::string> sdr_sources;
        std::string sdr_sources_str, source_id;
        std::string http_server = "0.0.0.0:8081";
        int selected_sdr = 0;
        bool fft_enable = true;

        // Import
        FileSelectWidget import_file = FileSelectWidget("importconfigfile", "Import Config");
        bool import_tracked_objects = false;
        bool import_rotator_settings = false;
        bool import_autotrack_settings = false;

    public:
        TrackingImportExport();
        bool draw_export();
        bool draw_import();

        void do_export(AutoTrackScheduler &scheduler, ObjectTracker &tracker, std::shared_ptr<rotator::RotatorHandler> rotator_handler);
        void do_import(AutoTrackScheduler &scheduler, ObjectTracker &tracker, std::shared_ptr<rotator::RotatorHandler> rotator_handler);
    };
} // namespace satdump