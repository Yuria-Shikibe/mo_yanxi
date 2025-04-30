module;

#include <GLFW/glfw3.h>
#include <windows.h>

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

		export
		[[nodiscard]] std::vector<std::string> get_drive_letters(){
			std::array<char, 256> buffer;
			const DWORD size = GetLogicalDriveStringsA(sizeof(buffer), buffer.data());
			std::vector<std::string> drives;
			for(char* drive = buffer.data(); drive < buffer.data() + size; drive += std::strlen(drive) + 1){
				drives.emplace_back(drive);
			}
			return drives;
		}

	}
}
