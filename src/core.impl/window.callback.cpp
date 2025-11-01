module;

#include <GLFW/glfw3.h>
#include <cassert>

module mo_yanxi.core.window.callback;

import mo_yanxi.core.window;
import mo_yanxi.core.global;
import mo_yanxi.core.global.ui;
import mo_yanxi.ui.root;
import mo_yanxi.core.ctrl.key_binding;

import mo_yanxi.input_handle;
import mo_yanxi.gui.global;


import std;

using namespace mo_yanxi::core;


void /*mo_yanxi::core::glfw::*/charInputCallback(glfw::Wptr window, unsigned codepoint){
	if(global::ui::root)global::ui::root->input_unicode(codepoint);
}

void mouseBottomCallBack(glfw::Wptr window, const int button, const int action, const int mods){
	{
		using namespace mo_yanxi::input_handle;

		mo_yanxi::gui::global::manager.input_key(key_set{static_cast<std::uint16_t>(button), static_cast<act>(action), static_cast<mode>(mods)});
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
}
