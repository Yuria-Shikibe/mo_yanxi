export module ext.Base64;

import std;

export namespace mo_yanxi::encode{
	constexpr std::uint8_t alphabet_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	constexpr std::uint8_t reverse_map[] = {
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 255, 255, 255,
			255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255,
			255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
			41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255
		};

	struct bad_code final : std::exception{
		bad_code() = default;

		explicit bad_code(char const* msg)
			: exception{msg}{}

		bad_code(char const* msg, const int i)
			: exception{msg, i}{}

		explicit bad_code(exception const& o)
			: exception{o}{}
	};

	template<typename RetContainer, std::ranges::sized_range Range>
		requires requires(RetContainer cont){
			requires std::ranges::random_access_range<Range>;
			requires sizeof(std::ranges::range_value_t<Range>) == 1;
			cont.push_back(std::uint8_t{});
		}
	[[nodiscard]] constexpr RetContainer encode(Range&& toEncode){
		RetContainer encoded{};

		const auto sz = std::ranges::size(toEncode);

		if constexpr (requires (RetContainer cont){
			cont.reserve(std::size_t{});
		}){
			encoded.reserve(sz * 4);
		}

		size_t i;
		for(i = 0; i + 3 <= sz; i += 3){
			encoded.push_back(alphabet_map[toEncode[i] >> 2]);
			encoded.push_back(alphabet_map[toEncode[i] << 4 & 0x30 | toEncode[i + 1] >> 4]);
			encoded.push_back(alphabet_map[toEncode[i + 1] << 2 & 0x3c | toEncode[i + 2] >> 6]);
			encoded.push_back(alphabet_map[toEncode[i + 2] & 0x3f]);
		}

		if(i < sz){
			if(const size_t tail = sz - i; tail == 1){
				encoded.push_back(alphabet_map[toEncode[i] >> 2]);
				encoded.push_back(alphabet_map[toEncode[i] << 4 & 0x30]);
				encoded.push_back('=');
				encoded.push_back('=');
			} else{
				encoded.push_back(alphabet_map[toEncode[i] >> 2]);
				encoded.push_back(alphabet_map[toEncode[i] << 4 & 0x30 | toEncode[i + 1] >> 4]);
				encoded.push_back(alphabet_map[toEncode[i + 1] << 2 & 0x3c]);
				encoded.push_back('=');
			}
		}

		return encoded;
	}

	template<typename RetContainer, std::ranges::sized_range Range>
	requires requires(RetContainer cont){
		requires std::ranges::random_access_range<Range>;
		requires sizeof(std::ranges::range_value_t<Range>) == 1;
		cont.push_back(std::uint8_t{});
	}
	[[nodiscard]] constexpr RetContainer decode(Range&& toDecode){
		if(std::ranges::size(toDecode) & 0x03 != 0){
			throw bad_code{"Decode Failed"};
		}

		RetContainer plain{};
		if constexpr(requires(RetContainer cont){
			cont.reserve(std::size_t{});
		}){
			plain.reserve(std::ranges::size(toDecode) / 4);
		}

		std::uint8_t quad[4];
		for(std::uint32_t i = 0; i < std::ranges::size(toDecode); i += 4){
			for(std::uint32_t k = 0; k < 4; k++){
				quad[k] = reverse_map[toDecode[i + k]];
			}

			if(!(quad[0] < 64 && quad[1] < 64)){
				throw bad_code{"Decode Failed"};
			}

			plain.push_back(quad[0] << 2 | quad[1] >> 4);

			if(quad[2] >= 64) break;
			if(quad[3] >= 64){
				plain.push_back(quad[1] << 4 | quad[2] >> 2);
				break;
			}
			plain.push_back(quad[1] << 4 | quad[2] >> 2);
			plain.push_back(quad[2] << 6 | quad[3]);
		}

		return plain;
	}
}
