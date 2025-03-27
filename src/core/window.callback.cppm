module;

#include <GLFW/glfw3.h>

export module mo_yanxi.core.window.callback;

namespace mo_yanxi::core::glfw{
	void windowRefreshCallback(GLFWwindow* window){}

	void dropCallback(GLFWwindow* window, int path_count, const char* paths[]) {}

	void charCallback(GLFWwindow* window, unsigned int codepoint);

	void charModCallback(GLFWwindow* window, unsigned int codepoint, int mods);

	void scaleCallback(GLFWwindow* window, const float xScale, const float yScale){}

	void mouseBottomCallBack(GLFWwindow* window, int button, int action, int mods);

	void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);

	void cursorEnteredCallback(GLFWwindow* window, int entered);

	void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods);

	void monitorCallback(GLFWmonitor* monitor, int event);

	void maximizeCallback(GLFWwindow* window, int maximized);

	void winPosCallBack(GLFWwindow* window, int xpos, int ypos);

	export void set_call_back(GLFWwindow* window, void* user);
}
