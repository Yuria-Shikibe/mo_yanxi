module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module mo_yanxi.vk.instance;

import mo_yanxi.vk.exception;
import mo_yanxi.vk.util;
import mo_yanxi.vk.validation;
import mo_yanxi.handle_wrapper;

import std;

namespace mo_yanxi::vk{
	/**
	 * @return All Required Extensions
	 */
	std::vector<const char*> get_required_extensions_glfw(){
		std::uint32_t glfwExtensionCount{};
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		return {glfwExtensions, glfwExtensions + glfwExtensionCount};
	}

	std::vector<const char*> get_required_extensions(){
		auto extensions = get_required_extensions_glfw();

		if constexpr(enable_validation_layers){
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);


		return extensions;
	}

	export constexpr VkApplicationInfo ApplicationInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};


	[[nodiscard]] bool checkValidationLayerSupport(const decltype(used_validation_layers)& validationLayers = used_validation_layers){
		auto [availableLayers, rst] = enumerate(vkEnumerateInstanceLayerProperties);
		return check_support(validationLayers, availableLayers, &VkLayerProperties::layerName);

	}

	export
	struct instance : exclusive_handle<VkInstance>{
	public:
		[[nodiscard]] constexpr instance() = default;

		[[nodiscard]] explicit instance(const VkApplicationInfo& app_info){
			init(app_info);
		}

	private:
		void init(const VkApplicationInfo& app_info){
			if(handle != nullptr){
				throw std::runtime_error("Instance is already initialized");
			}

			VkValidationFeaturesEXT validation_features_ext{
				.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
				.pNext = nullptr,
				.enabledValidationFeatureCount = 0,
				.pEnabledValidationFeatures = nullptr,
				.disabledValidationFeatureCount = static_cast<std::uint32_t>(disabled_validation_features.size()),
				.pDisabledValidationFeatures = disabled_validation_features.data()
			};

			VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
			// createInfo.pNext = &validation_features_ext;
			createInfo.pApplicationInfo = &app_info;

			if constexpr(enable_validation_layers){
				if(!checkValidationLayerSupport(used_validation_layers)){
					throw std::runtime_error("validation layers requested, but not available!");
				}

				std::println("[Vulkan] Using Validation Layer: ");

				for(const auto& validationLayer : used_validation_layers){
					std::println("\t{}", validationLayer);
				}

				createInfo.enabledLayerCount = static_cast<std::uint32_t>(used_validation_layers.size());
				createInfo.ppEnabledLayerNames = used_validation_layers.data();
			} else{
				createInfo.enabledLayerCount = 0;
			}

			const auto extensions = get_required_extensions();
			createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
			createInfo.ppEnabledExtensionNames = extensions.data();

			if(const auto rst = vkCreateInstance(&createInfo, nullptr, &handle)){
				throw vk_error(rst, "failed to create vulkan instance!");
			}

			std::println("[Vulkan] Instance Create Succeed: {:#X}", std::bit_cast<std::uintptr_t>(handle));
		}

	public:
		~instance(){
			vkDestroyInstance(handle, nullptr);
		}

		instance(const instance& other) = delete;
		instance(instance&& other) noexcept = default;
		instance& operator=(const instance& other) = delete;
		instance& operator=(instance&& other) noexcept = default;

		[[nodiscard]] operator VkInstance() const noexcept{ return handle; }
	};
}