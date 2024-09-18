#include "core/style.h"
#include "timed_message.h"
namespace widgets
{
	TimedMessage::TimedMessage()
	{
		start_time = nullptr;
		message = "";
	}
	TimedMessage::~TimedMessage()
	{
		delete start_time;
	}
	void TimedMessage::set_message(ImColor color, std::string message)
	{
		if(start_time == nullptr)
			start_time = new std::chrono::time_point<std::chrono::steady_clock>();
		*start_time = std::chrono::steady_clock::now();
		this->message = message;
		this->color = color;
	}
	void TimedMessage::draw()
	{
		if (start_time == nullptr)
			return;
		double time_running = std::chrono::duration<double>(std::chrono::steady_clock::now() - *start_time).count();
		if (time_running > 5)
		{
			delete start_time;
			start_time = nullptr;
			return;
		}
		else if (time_running > 4)
			color.Value.w = 1.0 - (time_running - 4.0);

		ImGui::SameLine();
		ImGui::TextColored(color, "%s", message.c_str());
	}
}