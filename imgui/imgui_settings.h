#include "imgui.h"

namespace c
{
	inline ImVec4 accent_color = ImColor(124, 103, 255, 255);

	namespace background
	{
		inline ImVec4 background = ImColor(12, 12, 12, 250);
		inline ImVec4 login_page = ImColor(0, 0, 0, 0);
		inline ImVec4 tab_list = ImColor(13, 14, 16, 255);
		inline ImVec4 stoke = ImColor(23, 21, 35, 255);
		inline ImVec2 size = ImVec2(700, 410);
		inline float rounding = 0;
	}	

	namespace child
	{
		inline ImVec4 background = ImColor(19, 19, 22, 150);
		inline ImVec4 outline = ImColor(23, 21, 35, 255);
		inline ImVec4 shadow = ImColor(19, 19, 22, 255);
		inline ImVec4 text = ImColor(255, 255, 255, 255);
		inline ImVec4 hint = ImColor(59, 62, 72, 255);

		inline ImVec2 spacing = ImVec2(15, 12);
		inline ImVec2 padding = ImVec2(15, 15);

		inline float rounding = 6.f;
	}

	namespace button
	{
		inline ImVec4 background = ImColor(20, 19, 23, 255);
		inline ImVec4 outline = ImColor(20, 19, 28, 255);
		inline float animation_speed = 7.5f;
		inline float rounding = 2.f;
	}

	namespace progress
	{
		inline ImVec4 background = ImColor(20, 19, 23, 220);
		inline ImVec4 outline = ImColor(20, 19, 28, 255);
		inline float animation_speed = 7.5f;
		inline float rounding = 4.f;
	}

	namespace input
	{
		inline ImVec4 background = ImColor(20, 19, 23, 255);
		inline ImVec4 outline = ImColor(20, 19, 28, 255);
		inline float animation_speed = 7.5f;
		inline float rounding = 2.f;
	}

	namespace text
	{
		inline ImVec4 text_active = ImColor(255, 255, 255, 255);
		inline ImVec4 text_hov = ImColor(88, 92, 104, 255);
		inline ImVec4 text = ImColor(59, 62, 72, 255);

		inline ImVec4 text_hint = ImColor(20, 19, 28, 255);
	}

	namespace tabs
	{
		inline float animation_speed = 7.5f;
		inline ImVec2 spacing = ImVec2(15, 15);
	}
}