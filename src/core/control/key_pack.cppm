//
// Created by Matrix on 2024/9/12.
//

export module mo_yanxi.core.ctrl.key_pack;

export import mo_yanxi.core.ctrl.constants;
import std;

namespace mo_yanxi::core::ctrl{
	constexpr unsigned KeyMask = 0xffff;

	export using packed_key_t = unsigned;

	/**
	* @code
	*	0b 0000'0000  0000'0000  0000'0000  0000'0000
	*	   [  Mode ]  [  Act  ]  [     Key Code     ]
	* @endcode
	*/
	export constexpr packed_key_t pack_key(const key_code_t keyCode, const key_code_t act, const key_code_t mode) noexcept{
		return
			(keyCode & KeyMask)
			| (act & act::Mask) << 16
			| (mode & mode::Mask) << (16 + act::Bits);
	}

	export  struct unpacked_key{
		key_code_t key;
		key_code_t act;
		key_code_t mode;

		[[nodiscard]] constexpr packed_key_t pack() const noexcept{
			return pack_key(key, act, mode);
		}

		constexpr friend bool operator==(const unpacked_key& lhs, const unpacked_key& rhs) noexcept = default;
	};



	/**
	 * @brief
	 * @return [keyCode, act, mode]
	 */
	export constexpr auto unpack_key(const packed_key_t fullKey) noexcept{
		return unpacked_key{
				.key = static_cast<key_code_t>(fullKey & KeyMask),
				.act = static_cast<key_code_t>(fullKey >> 16 & act::Mask),
				.mode = static_cast<key_code_t>(fullKey >> 24 & mode::Mask)
			};
	}

	export
	struct key_pack{
		packed_key_t code{};

		[[nodiscard]] constexpr key_pack() noexcept = default;

		[[nodiscard]] constexpr explicit(false) key_pack(const packed_key_t code)
			noexcept : code{code}{}

		[[nodiscard]] constexpr explicit(false) key_pack(const key_code_t keyCode, const key_code_t act, const key_code_t mode = mode::ignore)
			noexcept : code{pack_key(keyCode, act, mode)}{}

		[[nodiscard]] constexpr auto unpack() const noexcept{
			return unpack_key(code);
		}

		constexpr friend bool operator==(const key_pack& lhs, const key_pack& rhs) noexcept = default;

		[[nodiscard]] constexpr key_code_t key() const noexcept{
			return static_cast<int>(code & KeyMask);
		}

		[[nodiscard]] constexpr key_code_t action() const noexcept{
			return static_cast<int>((code >> 16) & act::Mask);
		}

		[[nodiscard]] constexpr key_code_t mode() const noexcept{
			return static_cast<int>((code >> 24) & act::Mask);
		}

		[[nodiscard]] constexpr bool matched(const key_pack state) const noexcept{
			auto [tk, ta, tm] = unpack();
			auto [ok, oa, om] = state.unpack();
			return key::matched(tk, ok) && act::matched(ta, oa) && act::matched(tm, om);
		}
	};
}
