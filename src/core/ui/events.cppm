//
// Created by Matrix on 2024/9/12.
//

export module mo_yanxi.ui.event_types;

export import mo_yanxi.event;
import mo_yanxi.math.vector2;
import mo_yanxi.core.ctrl.key_pack;
import std;

namespace mo_yanxi::ui{

	struct PosedEvent : mo_yanxi::events::event_type_tag{
		math::vec2 pos{};

		[[nodiscard]] PosedEvent() = default;

		[[nodiscard]] explicit PosedEvent(const math::vec2 pos)
			: pos{pos}{}
	};


	struct mouse_event : PosedEvent{
		core::ctrl::key_pack code{};

		[[nodiscard]] mouse_event() = default;

		[[nodiscard]] mouse_event(const math::vec2 pos, const core::ctrl::key_pack code)
			: PosedEvent{pos},
			  code{code}{}

		constexpr decltype(auto) unpack() const noexcept{
			return code.unpack();
		}
	};

	export namespace events{
		// template <typename T, typename Code = long long>
		// struct ConnectionEvent final : ext::event_type{
		// 	using TargetType = T;
		// 	using CodeType = Code;
		// 	std::vector<std::add_pointer_t<T>> targets{};
		// 	CodeType code{};
		//
		// 	[[nodiscard]] ConnectionEvent() = default;
		//
		// 	template <std::ranges::input_range Rng>
		// 	[[nodiscard]] explicit ConnectionEvent(Rng&& rng, CodeType code = {})
		// 		: targets(std::from_range, std::forward<Rng>(rng)), code{code}{}
		// };
		//
		// template <typename Code = long long>
		// struct CodeEvent final : ext::event_type{
		// 	using CodeType = Code;
		// 	CodeType code{};
		//
		// 	[[nodiscard]] CodeEvent() = default;
		//
		// 	[[nodiscard]] explicit CodeEvent(CodeType code = {})
		// 		: code{code}{}
		//
		// 	template <std::convertible_to<CodeType> T>
		// 		requires (std::is_enum_v<T>)
		// 	[[nodiscard]] explicit CodeEvent(T code)
		// 		: code{static_cast<CodeType>(code)}{}
		//
		// 	template <typename T>
		// 		requires (std::is_enum_v<T>)
		// 	explicit(false) operator CodeEvent<T>() const noexcept{
		// 		return CodeEvent<T>{static_cast<T>(code)};
		// 	}
		// };

		struct collapser_state_changed{
			 bool expanded;
		};

		struct check_box_state_changed{
			 std::size_t index;
		};



		// struct

		struct click final : mouse_event{
			using mouse_event::mouse_event;
		};

		struct cursor_moved final : PosedEvent{
			using PosedEvent::PosedEvent;
		};

		struct drag final : mouse_event{
			math::vec2 dst{};

			[[nodiscard]] drag() = default;

			[[nodiscard]] drag(const math::vec2 src, const math::vec2 dst, const core::ctrl::key_pack code)
				: mouse_event{src, code},
				  dst{dst}{}

			[[nodiscard]] constexpr math::vec2 trans() const noexcept{
				return dst - pos;
			}
		};

		struct inbound final : PosedEvent{using PosedEvent::PosedEvent;};
		struct exbound final : PosedEvent{using PosedEvent::PosedEvent;};

		struct focus_begin final : PosedEvent{using PosedEvent::PosedEvent;};
		struct focus_end final : PosedEvent{using PosedEvent::PosedEvent;};

		struct scroll final{
			[[nodiscard]] scroll() = default;

			[[nodiscard]] explicit scroll(const math::vec2 pos)
				: delta{pos}{}

			[[nodiscard]] scroll(const math::vec2 pos, core::ctrl::key_code_t mode)
				: delta{pos},
				  mode{mode}{}

			math::vec2 delta{};
			core::ctrl::key_code_t mode{};
		};

	}

}
