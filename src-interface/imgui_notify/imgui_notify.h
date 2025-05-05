// imgui-notify by patrickcjk
// https://github.com/patrickcjk/imgui-notify
/*
	MIT License

	Copyright (c) 2021 Patrick

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/
// 20223009: Orbiter - compile on linux + multiviewport + simplify & defuglyfy

#ifndef IMGUI_NOTIFY
#define IMGUI_NOTIFY

#include <vector>
#include <string>
#include "imgui/imgui.h"
#include "common/imgui_utils.h"
#include <chrono>
#include <algorithm>
#include <stdarg.h>
#include "font_awesome_5.h"

using namespace std::chrono_literals;
using duration = std::chrono::duration<double>;
using steady_clock = std::chrono::steady_clock;

const duration NOTIFY_FADE_IN_OUT_TIME = 0.15s; // Fade in and out duration
const duration NOTIFY_DEFAULT_DISMISS = 7.0s;	// Auto dismiss (default, applied only of no data provided in constructors)
const float NOTIFY_PADDING_X = 20.f;			// Bottom-left X padding
const float NOTIFY_PADDING_Y = 20.f;			// Bottom-left Y padding
const float NOTIFY_PADDING_MESSAGE_Y = 10.f;	// Padding Y between each message
const float NOTIFY_OPACITY = 1.0f;				// 0-1 Toast opacity
const ImGuiWindowFlags NOTIFY_TOAST_FLAGS = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings;

// Comment out if you don't want any separator between title and content
#define NOTIFY_USE_SEPARATOR

typedef int ImGuiToastType;
typedef int ImGuiToastPhase;
typedef int ImGuiToastPos;

enum ImGuiToastType_
{
	ImGuiToastType_None,
	ImGuiToastType_Success,
	ImGuiToastType_Warning,
	ImGuiToastType_Error,
	ImGuiToastType_Info,
	ImGuiToastType_COUNT
};

enum ImGuiToastPhase_
{
	ImGuiToastPhase_FadeIn,
	ImGuiToastPhase_Wait,
	ImGuiToastPhase_FadeOut,
	ImGuiToastPhase_Expired,
	ImGuiToastPhase_COUNT
};

enum ImGuiToastPos_
{
	ImGuiToastPos_TopLeft,
	ImGuiToastPos_TopCenter,
	ImGuiToastPos_TopRight,
	ImGuiToastPos_BottomLeft,
	ImGuiToastPos_BottomCenter,
	ImGuiToastPos_BottomRight,
	ImGuiToastPos_Center,
	ImGuiToastPos_COUNT
};

class ImGuiToast
{
public:
	ImGuiToastType type = ImGuiToastType_None;
	std::string title;
	std::string content;
	duration dismiss_time = NOTIFY_DEFAULT_DISMISS;
	std::chrono::time_point<steady_clock> creation_time;

	ImVec4 get_color() const
	{
		switch (type)
		{
		case ImGuiToastType_Success:
			return {0, 255, 0, 255}; // Green
		case ImGuiToastType_Warning:
			return {255, 255, 0, 255}; // Yellow
		case ImGuiToastType_Error:
			return {255, 0, 0, 255}; // Red
		case ImGuiToastType_Info:
			return {0, 157, 255, 255}; // Blue
		case ImGuiToastType_None:
		default:
			return { 255, 255, 255, 255 }; // White
		}
		assert(false);
	}

	const char *get_icon()
	{
		switch (type)
		{
		case ImGuiToastType_Success:
			return ICON_FA_CHECK_CIRCLE;
		case ImGuiToastType_Warning:
			return ICON_FA_EXCLAMATION_TRIANGLE;
		case ImGuiToastType_Error:
			return ICON_FA_TIMES_CIRCLE;
		case ImGuiToastType_Info:
			return ICON_FA_INFO_CIRCLE;
		case ImGuiToastType_None:
		default:
			return nullptr;
		}
		assert(false);
	}

	duration get_elapsed_time() const { return steady_clock::now() - creation_time; }

	ImGuiToastPhase get_phase() const
	{
		const auto elapsed = get_elapsed_time();

		if (elapsed > NOTIFY_FADE_IN_OUT_TIME + dismiss_time + NOTIFY_FADE_IN_OUT_TIME)
		{
			return ImGuiToastPhase_Expired;
		}
		else if (elapsed > NOTIFY_FADE_IN_OUT_TIME + dismiss_time)
		{
			return ImGuiToastPhase_FadeOut;
		}
		else if (elapsed > NOTIFY_FADE_IN_OUT_TIME)
		{
			return ImGuiToastPhase_Wait;
		}
		else
		{
			return ImGuiToastPhase_FadeIn;
		}
	}

	float get_fade_percent() const
	{
		const auto phase = get_phase();
		duration elapsed = get_elapsed_time();

		if (phase == ImGuiToastPhase_FadeIn)
		{
			return (elapsed / NOTIFY_FADE_IN_OUT_TIME) * NOTIFY_OPACITY;
		}
		else if (phase == ImGuiToastPhase_FadeOut)
		{
			return (1.f - ((elapsed - NOTIFY_FADE_IN_OUT_TIME - dismiss_time) / NOTIFY_FADE_IN_OUT_TIME)) * NOTIFY_OPACITY;
		}

		return 1.f * NOTIFY_OPACITY;
	}

public:
	// Constructors

	ImGuiToast(ImGuiToastType t, const char *ttl, const char *msg, duration dt = NOTIFY_DEFAULT_DISMISS)
	{
		IM_ASSERT(type < ImGuiToastType_COUNT);
		creation_time = steady_clock::now();
		type = t;
		dismiss_time = dt;
		title = ttl;
		content = msg;
	}
};

namespace ImGui
{
	inline std::vector<ImGuiToast> notifications;

	/// <summary>
	/// Insert a new toast in the list
	/// </summary>
	inline void InsertNotification(const ImGuiToast &toast)
	{
		notifications.push_back(toast);
	}

	/// <summary>
	/// Remove a toast from the list by its index
	/// </summary>
	/// <param name="index">index of the toast to remove</param>
	inline void RemoveNotification(int index)
	{
		notifications.erase(notifications.begin() + index);
	}

	/// <summary>
	/// Render toasts, call at the end of your rendering!
	/// </summary>
	inline void RenderNotifications()
	{
		auto vp_size = GetMainViewport()->Size;
		vp_size.x += GetMainViewport()->Pos.x;
		vp_size.y += GetMainViewport()->Pos.y;

		float height = 0.f;

		// Remove toasts if expired
		notifications.erase(std::remove_if(notifications.begin(),
										   notifications.end(),
										   [=](auto &toast)
										   { return toast.get_phase() == ImGuiToastPhase_Expired; }),
							notifications.end());

		int i = 0;
		for (auto &current_toast : notifications)
		{
			// Get icon, title and other data
			// const auto icon = t2i(current_toast.type);
			const auto icon = current_toast.get_icon();
			const auto title = current_toast.title;
			const auto content = current_toast.content;
			const auto opacity = current_toast.get_fade_percent(); // Get opacity based of the current phase

			// Window rendering
			auto text_color = current_toast.get_color();
			text_color.w = opacity;

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, opacity);

			SetNextWindowPos(ImVec2(vp_size.x - NOTIFY_PADDING_X, vp_size.y - NOTIFY_PADDING_Y - height), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

			// Generate new unique name for this toast
			char window_name[50];
			sprintf(window_name, "##TOAST%d", i);
			i++;
			Begin(window_name, NULL, NOTIFY_TOAST_FLAGS);
			ImGuiUtils_BringCurrentWindowToFront();

			// Here we render the toast content
			{
				PushTextWrapPos(vp_size.x / 3.f); // We want to support multi-line text, this will wrap the text after 1/3 of the screen width

				bool was_title_rendered = false;

				// If an icon is set
				if (icon)
				{
					TextColored(text_color, "%s", icon);
					was_title_rendered = true;
				}

				// If a title is set
				if (!title.empty())
				{
					// If a title and an icon is set, we want to render on same line
					if (icon)
						SameLine();

					TextUnformatted(title.c_str()); // Render title text
					was_title_rendered = true;
				}

				// In case ANYTHING was rendered in the top, we want to add a small padding so the text (or icon) looks centered vertically
				if (was_title_rendered && !content.empty())
				{
					SetCursorPosY(GetCursorPosY() + 5.f); // Must be a better way to do this!!!!
				}

				// If a content is set
				if (!content.empty())
				{
					if (was_title_rendered)
					{
#ifdef NOTIFY_USE_SEPARATOR
						Separator();
#endif
					}

					TextUnformatted(content.c_str()); // Render content text
				}

				PopTextWrapPos();
			}
			ImGui::PopStyleVar();
			// Save height for next toasts
			height += GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;

			// End
			End();
		}
	}
}

#endif
