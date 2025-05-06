//
// Created by Matrix on 2025/5/5.
//

export module mo_yanxi.game.component.hitbox_meta;

export import mo_yanxi.math.quad;
import std;

export namespace mo_yanxi::game{

	struct hitbox_meta{
		struct meta{
			math::rect_box_identity<float> box;
			math::trans2 trans;
		};

		std::vector<meta> components{};

		constexpr decltype(auto) operator[](this auto&& self, const std::size_t index) noexcept{
			return self.components[index];
		}

		template <typename S>
		constexpr auto begin(this S&& self) noexcept{
			return std::ranges::begin(std::forward_like<S>(self.components));
		}

		template <typename S>
		constexpr auto end(this S&& self) noexcept{
			return std::ranges::end(std::forward_like<S>(self.components));
		}

		[[nodiscard]] constexpr std::size_t size() const noexcept{
			return components.size();
		}
	};
}