#include <vulkan/vulkan.h>

import std;
import mo_yanxi.math;
import mo_yanxi.math.angle;
import mo_yanxi.math.vector2;
import mo_yanxi.math.vector4;
import mo_yanxi.vk.object.instance;

int main(){
	using namespace mo_yanxi;

	vk::instance instance{};
	instance = vk::instance{vk::ApplicationInfo};
	std::println("{}", "end");
}
