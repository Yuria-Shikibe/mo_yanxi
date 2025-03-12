module;

#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE

#include <Windows.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

export module mo_yanxi.core.window;
import std;
import mo_yanxi.core.window.callback;
// import mo_yanxi.math.vector2;
import mo_yanxi.event;
import mo_yanxi.handle_wrapper;
import mo_yanxi.vk.exception;

namespace mo_yanxi{
	constexpr std::uint32_t WIDTH = 800;
	constexpr std::uint32_t HEIGHT = 800;

	export namespace core::glfw{
		void init(){
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			// glfwWindowHint(GLFW_REFRESH_RATE, 60);
		}

		void terminate(){
			glfwTerminate();
		}
	}

	export
	struct window_instance{
		struct resize_event final : events::event_type_tag{
			VkExtent2D size{};

			[[nodiscard]] explicit resize_event(const VkExtent2D size)
				: size(size){
			}
		};

	public:
		[[nodiscard]] window_instance() = default;


		[[nodiscard]] window_instance(const char* name, bool full_screen = false){
			//TODO full screen support
			handle = glfwCreateWindow(WIDTH, HEIGHT, name, nullptr, nullptr);
			core::glfw::set_call_back(handle, this);

			int x;
			int y;
			glfwGetWindowSize(handle, &x, &y);
			size = VkExtent2D{static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)};
		}

		~window_instance(){
			if(handle){
				glfwDestroyWindow(handle);
			}
		}

		[[nodiscard]] bool iconified() const noexcept{
			return glfwGetWindowAttrib(handle, GLFW_ICONIFIED) || (size.width == 0 || size.height == 0);
		}

		[[nodiscard]] bool should_close() const noexcept{
			return glfwWindowShouldClose(handle);
		}

	    void on_resize(const std::string_view name, std::move_only_function<void(const resize_event&) const>&& callback) {
		    eventManager.on<resize_event>(name, std::move(callback));
		}

		void poll_events() const noexcept{
			glfwPollEvents();
		}

		void wait_event() const noexcept{
			glfwWaitEvents();
		}

		[[nodiscard]] VkSurfaceKHR create_surface(VkInstance instance) const{
			VkSurfaceKHR surface{};
			if(const auto rst = glfwCreateWindowSurface(instance, handle, nullptr, &surface)){
				throw vk::vk_error(rst, "Failed to create window surface!");
			}

			return surface;
		}


		[[nodiscard]] GLFWwindow* get_handle() const noexcept{ return handle; }

		[[nodiscard]] VkExtent2D get_size() const noexcept{
			return size;
		}

		static void native_resize_callback(GLFWwindow* window, int width, int height){
			const auto app = static_cast<window_instance*>(glfwGetWindowUserPointer(window));

			app->resize(width, height);
		}

		[[nodiscard]] constexpr bool check_resized() noexcept{
			return std::exchange(lazy_resized_check, false);
		}

	private:
		void resize(const int w, const int h){
			if(w == size.width && h == size.height)return;
			size = VkExtent2D{static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h)};
			lazy_resized_check = true;
			eventManager.fire(resize_event{get_size()});
		}

	public:
		window_instance(const window_instance& other) = delete;
		window_instance& operator=(const window_instance& other) = delete;

		window_instance(window_instance&& other) noexcept
			: eventManager{std::move(other.eventManager)},
			  handle{std::move(other.handle)},
			  size{std::move(other.size)}{
			core::glfw::set_call_back(handle, this);
		}

		window_instance& operator=(window_instance&& other) noexcept{
			if(this == &other) return *this;
			eventManager = std::move(other.eventManager);
			handle = std::move(other.handle);
			size = std::move(other.size);
			core::glfw::set_call_back(handle, this);
			return *this;
		}

		HWND get_native() const noexcept{
			return glfwGetWin32Window(handle);
		}

	private:
		events::named_event_manager<std::move_only_function<void() const>, resize_event> eventManager{};

		exclusive_handle_member<GLFWwindow*> handle{};
		VkExtent2D size{};
		bool lazy_resized_check{};
	};
}