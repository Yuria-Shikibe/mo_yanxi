module;

#include <GLFW/glfw3.h>
#include <cassert>

module mo_yanxi.core.window.callback;

import mo_yanxi.core.window;
// import Core.Input;
// import Core.Global;
// import Core.UI.Root;
// import Core.Global.UI;
// import Geom.Vector2D;
import std;

void mo_yanxi::core::glfw::charCallback(GLFWwindow* window, unsigned int codepoint){
	// if(Global::UI::root)Global::UI::root->inputUnicode(codepoint);
}


void mo_yanxi::core::glfw::mouseBottomCallBack(GLFWwindow* window, const int button, const int action, const int mods){
	// Global::input->informMouseAction(button, action, mods);
	// if(Global::UI::root)Global::UI::root->inputMouse(button, action, mods);
}

void mo_yanxi::core::glfw::cursorPosCallback(GLFWwindow* window, const double xPos, const double yPos){
	// const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
	// Geom::Vec2 pos{static_cast<float>(xPos), static_cast<float>(app->getSize().y - yPos)};
	// Global::input->cursorMoveInform(pos.x, pos.y);
	// if(Global::UI::root)Global::UI::root->cursorPosUpdate(pos.x, pos.y);
}

void mo_yanxi::core::glfw::cursorEnteredCallback(GLFWwindow* window, const int entered){
	// Global::input->setInbound(entered);
}

void mo_yanxi::core::glfw::scrollCallback(GLFWwindow* window, const double xOffset, const double yOffset){
	// Global::input->setScrollOffset(static_cast<float>(xOffset), static_cast<float>(yOffset));
}

void mo_yanxi::core::glfw::keyCallback(GLFWwindow* window, const int key, const int scanCode, const int action, const int mods){
	// if(key >= 0 && key < GLFW_KEY_LAST){
	// 	assert(Global::input != nullptr);
	// 	assert(Global::UI::root != nullptr);
	// 	Global::input->informKeyAction(key, scanCode, action, mods);
	// 	Global::UI::root->inputKey(key, action, mods);
	// }
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
