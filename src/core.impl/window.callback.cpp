module;

#include <GLFW/glfw3.h>
#include <cassert>

module mo_yanxi.core.window.callback;

import mo_yanxi.font.typesetting;

import mo_yanxi.core.window;
import mo_yanxi.core.global;
import mo_yanxi.core.global.ui;
import mo_yanxi.ui.root;
import mo_yanxi.core.ctrl.key_binding;

import mo_yanxi.input_handle;
import mo_yanxi.gui.global;
import mo_yanxi.math.vector2;


import std;

using namespace mo_yanxi::core;

GLFWmonitor* GetMonitorFromWindow(GLFWwindow* window){
	if(!window) return nullptr;

	// 1. 获取窗口的位置和尺寸
	int wx, wy;
	glfwGetWindowPos(window, &wx, &wy);
	int ww, wh;
	glfwGetWindowSize(window, &ww, &wh);

	int bestOverlap = 0;
	GLFWmonitor* bestMonitor = nullptr;

	// 2. 获取所有连接的显示器
	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);

	if(count == 1){
		return monitors[0];
	}

	for(int i = 0; i < count; ++i){
		GLFWmonitor* monitor = monitors[i];

		// 3. 获取显示器的位置和当前视频模式（分辨率）
		int mx, my;
		glfwGetMonitorPos(monitor, &mx, &my);
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		// 显示器的边界定义为 [mx, my] 到 [mx + mode->width, my + mode->height]

		// 计算重叠区域
		// 重叠矩形的左上角坐标 (Intersection)
		int intersectX1 = std::max(wx, mx);
		int intersectY1 = std::max(wy, my);

		// 重叠矩形的右下角坐标
		int intersectX2 = std::min(wx + ww, mx + mode->width);
		int intersectY2 = std::min(wy + wh, my + mode->height);

		// 计算重叠的宽度和高度
		int overlapW = std::max(0, intersectX2 - intersectX1);
		int overlapH = std::max(0, intersectY2 - intersectY1);

		// 计算重叠面积
		int currentOverlap = overlapW * overlapH;

		// 4. 选出重叠面积最大的显示器
		if(currentOverlap > bestOverlap){
			bestOverlap = currentOverlap;
			bestMonitor = monitor;
		}
	}

	return bestMonitor;
}

// --- 主要函数：计算 PPI ---
mo_yanxi::math::vec2 CalculateWindowPPI(GLFWwindow* window) {
    GLFWmonitor* monitor = GetMonitorFromWindow(window);

    if (!monitor) {
        return {102, 102};
    }

    // 1. 获取物理尺寸 (毫米)
    int widthMM, heightMM;
    glfwGetMonitorPhysicalSize(monitor, &widthMM, &heightMM);

    // 2. 获取当前分辨率 (像素)
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    const auto widthPx = mode->width;
    const auto heightPx = mode->height;

    // 3. 计算 PPI (确保物理尺寸有效)
    if (widthMM > 0 && heightMM > 0) {
        // 1 英寸 = 25.4 毫米
        float ppiX = static_cast<float>(widthPx) / (static_cast<float>(widthMM) / 25.4f);
        float ppiY = static_cast<float>(heightPx) / (static_cast<float>(heightMM) / 25.4f);
    	return {ppiX, ppiY};
    }

	return {102, 102};
}

void /*mo_yanxi::core::glfw::*/charInputCallback(glfw::Wptr window, unsigned codepoint){
	if(global::ui::root)global::ui::root->input_unicode(codepoint);
}

void mouseBottomCallBack(glfw::Wptr window, const int button, const int action, const int mods){
	{
		using namespace mo_yanxi::input_handle;

		mo_yanxi::gui::global::manager.input_mouse(key_set{static_cast<std::uint16_t>(button), static_cast<act>(action), static_cast<mode>(mods)});
	}
	global::input.inform_mouse_action(button, action, mods);
	if(global::ui::root)global::ui::root->input_mouse(button, action, mods);
}

void cursorPosCallback(glfw::Wptr window, const double xPos, const double yPos){
	// const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
	// Geom::Vec2 pos{static_cast<float>(xPos), static_cast<float>(app->getSize().y - yPos)};
	mo_yanxi::gui::global::manager.cursor_pos_update(xPos, yPos);

	global::input.cursor_move_inform(xPos, yPos);
	if(global::ui::root)global::ui::root->cursor_pos_update(xPos, yPos);
}

void cursorEnteredCallback(glfw::Wptr window, const int entered){
	global::input.set_inbound(entered);
}

void scrollCallback(glfw::Wptr window, const double xOffset, const double yOffset){
	mo_yanxi::gui::global::manager.scroll_update(xOffset, yOffset);

	global::input.set_scroll_offset(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void keyCallback(glfw::Wptr window, const int key, const int scanCode, const int action, const int mods){
	if(key >= 0 && key < GLFW_KEY_LAST){
	// 	assert(Global::input != nullptr);
	// 	assert(Global::UI::root != nullptr);

		{
			using namespace mo_yanxi::input_handle;
			// std::println(std::cerr, "Act: {}, Mode: {}: {}", magic_enum::enum_name(static_cast<act>(action)), magic_enum::enum_name<mode, magic_enum::detail::enum_subtype::flags>(static_cast<mode>(mods)), mods);
			mo_yanxi::gui::global::manager.input_key(key_set{static_cast<std::uint16_t>(key), static_cast<act>(action), static_cast<mode>(mods)});

		}
		global::input.inform_key_action(key, scanCode, action, mods);
		if(global::ui::root)global::ui::root->input_key(key, action, mods);

	// 	Global::UI::root->inputKey(key, action, mods);
	}
}



void glfw::set_call_back(Wptr window, void* user){
	glfwSetWindowUserPointer(window, user);
	glfwSetFramebufferSizeCallback(window, window_instance::native_resize_callback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetMouseButtonCallback(window, mouseBottomCallBack);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetCursorEnterCallback(window, cursorEnteredCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCharCallback(window, charInputCallback);

	font::typesetting::glyph_size::screen_ppi = CalculateWindowPPI(window);
}
