module;

#include <GLFW/glfw3.h>

export module mo_yanxi.core.platform;

import std;

namespace mo_yanxi::core{
	namespace glfw{

	}

	namespace plat{
		export std::string_view get_clipboard(GLFWwindow* wwindow){
			return glfwGetClipboardString(wwindow);
		}

		export void set_clipborad(GLFWwindow* wwindow, const char* p){
			glfwSetClipboardString(wwindow, p);
		}
	}
}
