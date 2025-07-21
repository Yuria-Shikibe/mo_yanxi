module;

#include <cassert>
#include <GLFW/glfw3.h>

export module mo_yanxi.core.application_timer;

export import mo_yanxi.core.unit;
import std;

namespace mo_yanxi::core{
	namespace glfw{
		double getTime(){
			return (glfwGetTime());
		}

		void resetTime(const double t){
			glfwSetTime(t);
		}

		double getDelta(const double last){
			return (glfwGetTime()) - last;
		}
	}

	export
	template <typename T>
		requires (std::is_floating_point_v<T>)
    // using T = float;
	class application_timer{
		using RawSec = double;
		using RawTick = direct_access_time_unit<double, tick_ratio>;

		using Tick = direct_access_time_unit<T, tick_ratio>;
		using Sec = direct_access_time_unit<T>;

	    RawSec globalTime{};
	    RawSec globalDelta{};
	    RawSec updateTime{};
	    RawSec updateDelta{};

		delta_setter deltaSetter{nullptr};
		timer_setter timerSetter{nullptr};
	    time_reseter timeReseter{nullptr};

		bool paused = false;
	    //TODO time scale?

	public:
		static constexpr T ticks_per_second{tick_ratio::den};

        [[nodiscard]] constexpr application_timer() noexcept = default;

        [[nodiscard]] constexpr application_timer(
        	const delta_setter deltaSetter,
        	const timer_setter timerSetter,
        	const time_reseter timeReseter
        ) noexcept :
            deltaSetter{deltaSetter},
            timerSetter{timerSetter},
            timeReseter{timeReseter} {}

		constexpr static application_timer get_default() noexcept{
	        return application_timer{
				glfw::getDelta, glfw::getTime, glfw::resetTime
	        };
        }

        constexpr void pause() noexcept {paused = true;}

		[[nodiscard]] constexpr bool is_paused() const noexcept{return paused;}

		constexpr void resume() noexcept {paused = false;}

		constexpr void set_pause(const bool v) noexcept {paused = v;}

		void fetch_time(){
#if DEBUG_CHECK
			if(!timerSetter || !deltaSetter){
				throw std::invalid_argument{"Missing Timer Function Pointer"};
			}
#endif

			globalDelta = std::clamp<double>(deltaSetter(globalTime), 0, 5. / 60.);
			globalTime = timerSetter();

			updateDelta = paused ? RawSec{0} : globalDelta;
			updateTime += updateDelta;
		}

		void reset_time() {
        	assert(timeReseter != nullptr);

			updateTime = globalTime = 0;
		    timeReseter(0);
		}

		[[nodiscard]] constexpr double raw_global_time() const noexcept{return globalTime;}


		[[nodiscard]] constexpr Sec global_delta() const noexcept{return Sec{static_cast<T>(globalDelta)};}
		[[nodiscard]] constexpr Sec update_delta() const noexcept{return Sec{static_cast<T>(updateDelta)};}

		[[nodiscard]] constexpr Sec global_time() const noexcept{return Sec{static_cast<T>(globalTime)};}
		[[nodiscard]] constexpr Sec update_time() const noexcept{return Sec{static_cast<T>(updateTime)};}

		[[nodiscard]] constexpr Tick global_delta_tick() const noexcept{return Tick{static_cast<T>(globalDelta * tick_ratio::den)};}
		[[nodiscard]] constexpr Tick update_delta_tick() const noexcept{return Tick{static_cast<T>(updateDelta * tick_ratio::den)};}

	};
	

	
}
