#include "timed_message.h"
namespace widgets
{
	TimedMessage::TimedMessage(ImColor color, int time) : color(color), time(time)
	{
		start_time = nullptr;
		message = "";
	}
	TimedMessage::~TimedMessage()
	{
		delete start_time;
	}
	void TimedMessage::set_message(std::string message)
	{
		if(start_time == nullptr)
			start_time = new std::chrono::time_point<std::chrono::steady_clock>();
		*start_time = std::chrono::steady_clock::now();
		this->message = message;
		color.Value.w = 1.0f;
	}
	void TimedMessage::draw()
	{
		if (start_time == nullptr)
			return;
		double time_running = std::chrono::duration<double>(std::chrono::steady_clock::now() - *start_time).count();
		if (time_running > time + 1)
		{
			delete start_time;
			start_time = nullptr;
			return;
		}
		else if (time_running > time)
			color.Value.w = 1.0 - (time_running - (double)time);

		ImGui::SameLine();
		ImGui::TextColored(color, "%s", message.c_str());
	}
}