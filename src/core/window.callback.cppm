module;

#include <GLFW/glfw3.h>

export module mo_yanxi.core.window.callback;

namespace mo_yanxi::core::glfw{
	using Wptr = GLFWwindow*;
	void windowRefreshCallback(Wptr window){}

	void dropCallback(Wptr window, int path_count, const char* paths[]) {}

	// void charInputCallback(Wptr window, unsigned codepoint);

	void charModCallback(Wptr window, unsigned int codepoint, int mods){}

	void scaleCallback(Wptr window, const float xScale, const float yScale){}

	// void mouseBottomCallBack(Wptr window, int button, int action, int mods);
	//
	// void cursorPosCallback(Wptr window, double xPos, double yPos);
	//
	// void cursorEnteredCallback(Wptr window, int entered);
	//
	// void scrollCallback(Wptr window, double xOffset, double yOffset);
	//
	// void keyCallback(Wptr window, int key, int scanCode, int action, int mods);

	void monitorCallback(GLFWmonitor* monitor, int event){}

	void maximizeCallback(Wptr window, int maximized){}

	void winPosCallBack(Wptr window, int xpos, int ypos){}

	export void set_call_back(Wptr window, void* user);
}
