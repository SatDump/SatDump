#pragma once
#include <chrono>
#include <string>
#include "imgui/imgui.h"

namespace widgets
{
	class TimedMessage
	{
	private:
		ImColor color;
		int time;
		std::chrono::time_point<std::chrono::steady_clock> *start_time;
		std::string message;
	public:
		TimedMessage(ImColor color, int time);
		~TimedMessage();
		void set_message(std::string message);
		void draw();
	};
}