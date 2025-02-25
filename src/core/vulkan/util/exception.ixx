module;

#include <vulkan/vulkan.h>

export module mo_yanxi.vk.exception;

import std;

namespace mo_yanxi::vk{
	export struct exception final : std::exception{
		[[nodiscard]] exception(const VkResult result, const std::string_view msg)
			: std::exception(msg.data(), result), result{result}{
		}

		VkResult result{};

		[[nodiscard]] const char* what() const override{
			return std::exception::what();
		}
	};

	export struct unqualified_error final : std::exception{
		[[nodiscard]] unqualified_error() : std::exception("Env/Device Disabled"){

		}

		[[nodiscard]] unqualified_error(const std::string_view msg)
			: std::exception(msg.data()){
		}

		[[nodiscard]] const char* what() const override{
			return std::exception::what();
		}
	};
}
