#include "import_export.h"
#include "imgui/imgui.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/widgets/json_editor.h"

namespace satdump
{
	// Export Settings
	int selected_sdr = 0;
	bool is_init = false;
	std::shared_ptr<dsp::DSPSampleSource> source_obj;
	std::vector<std::string> sdr_sources;
	std::string sdr_sources_str;

	// Import Settings

	void export_tracking()
	{
		//Get SDRs
		ImGui::SeparatorText("SDR Settings");
		if (!is_init)
		{
			source_obj = dsp::dsp_sources_registry.begin()->second.getInstance({
				dsp::dsp_sources_registry.begin()->first, "", "" });

			for (auto& this_source : dsp::dsp_sources_registry)
			{
				if (this_source.first == "remote")
					continue;
				sdr_sources.push_back(this_source.first);
				sdr_sources_str += this_source.first + '\0';
			}
			sdr_sources_str += '\0';
			is_init = true;
		}
		if (ImGui::Combo("Source", &selected_sdr, sdr_sources_str.c_str()))
			source_obj = dsp::dsp_sources_registry[sdr_sources[selected_sdr]]
				.getInstance({sdr_sources[selected_sdr], "", "" });

		//Get SDR Settings
		source_obj->drawControlUI();
	}
}