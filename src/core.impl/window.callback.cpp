module;

#include <GLFW/glfw3.h>
#include <cassert>

module mo_yanxi.core.window.callback;

import mo_yanxi.core.window;
import mo_yanxi.core.global;
import mo_yanxi.core.global.ui;
import mo_yanxi.ui.root;
import mo_yanxi.core.ctrl.key_binding;
// import Core.UI.Root;
// import Core.Global.UI;
// import Geom.Vector2D;
import std;

void mo_yanxi::core::glfw::charCallback(GLFWwindow* window, unsigned int codepoint){
	if(global::ui::root)global::ui::root->input_unicode(codepoint);
}


void mo_yanxi::core::glfw::mouseBottomCallBack(GLFWwindow* window, const int button, const int action, const int mods){
	global::input.inform_mouse_action(button, action, mods);
	if(global::ui::root)global::ui::root->input_mouse(button, action, mods);
}

void mo_yanxi::core::glfw::cursorPosCallback(GLFWwindow* window, const double xPos, const double yPos){
	// const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
	// Geom::Vec2 pos{static_cast<float>(xPos), static_cast<float>(app->getSize().y - yPos)};
	global::input.cursor_move_inform(xPos, yPos);
	if(global::ui::root)global::ui::root->cursor_pos_update(xPos, yPos);
}

void mo_yanxi::core::glfw::cursorEnteredCallback(GLFWwindow* window, const int entered){
	global::input.set_inbound(entered);
}

void mo_yanxi::core::glfw::scrollCallback(GLFWwindow* window, const double xOffset, const double yOffset){
	global::input.set_scroll_offset(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void mo_yanxi::core::glfw::keyCallback(GLFWwindow* window, const int key, const int scanCode, const int action, const int mods){
	if(key >= 0 && key < GLFW_KEY_LAST){
	// 	assert(Global::input != nullptr);
	// 	assert(Global::UI::root != nullptr);
		global::input.inform_key_action(key, scanCode, action, mods);
		if(global::ui::root)global::ui::root->input_key(key, action, mods);

	// 	Global::UI::root->inputKey(key, action, mods);
	}
}



void mo_yanxi::core::glfw::set_call_back(GLFWwindow* window, void* user){
	glfwSetWindowUserPointer(window, user);
	glfwSetFramebufferSizeCallback(window, window_instance::native_resize_callback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetMouseButtonCallback(window, mouseBottomCallBack);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetCursorEnterCallback(window, cursorEnteredCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCharCallback(window, charCallback);
}
