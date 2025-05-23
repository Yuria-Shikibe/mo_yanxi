module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.color;


import std;
import mo_yanxi.math;
export import mo_yanxi.math.vector4;

namespace mo_yanxi::graphic{
	/**
	 * \brief  32Bits for 4 u byte[0, 255]
	 * \code
	 * 00000000__00000000__00000000__00000000
	 * r value^  g value^  b value^  a value^
	 *       24        16         8         0
	 * \endcode
	 *
	 * TODO HDR support
	 */
	export
	struct color : math::vector4<float>{
	public:
		static constexpr float max_luma_scale = 3.f;
		static constexpr float light_color_range = 2550.f;

		static constexpr auto max_val = std::numeric_limits<std::uint8_t>::max();
		static constexpr float max_val_f = std::numeric_limits<std::uint8_t>::max();
		static constexpr unsigned int r_Offset = 24;
		static constexpr unsigned int g_Offset = 16;
		static constexpr unsigned int b_Offset = 8;
		static constexpr unsigned int a_Offset = 0;
		static constexpr unsigned int a_Mask = 0x00'00'00'ff;
		static constexpr unsigned int b_Mask = 0x00'00'ff'00;
		static constexpr unsigned int g_Mask = 0x00'ff'00'00;
		static constexpr unsigned int r_Mask = 0xff'00'00'00;

		using rgba8_bits = unsigned int;

		// constexpr color() noexcept : color(1, 1, 1, 1){
		// }
		//
		// constexpr explicit color(const rgba8_bits rgba8888V) noexcept{
		// 	from_rgba8888(rgba8888V);
		// }
		//
		// constexpr color(const float r, const float g, const float b, const float a) noexcept
		// 	: vector4<float>(r, g, b, a){
		// }
		//
		// constexpr color(const float r, const float g, const float b) noexcept : color(r, g, b, 1){
		// }

	private:
		template <bool doClamp>
		FORCE_INLINE constexpr color& clampCond() noexcept{
			if constexpr(doClamp){
				return clamp();
			} else{
				return *this;
			}
		}

	public:
		//TODO is this really good??
		FORCE_INLINE constexpr color& append_light_color(const color& color) noexcept{
			r += static_cast<float>(math::floor((light_color_range - 10) * color.r, 10) + 10);
			g += static_cast<float>(math::floor((light_color_range - 10) * color.g, 10) + 10);
			b += static_cast<float>(math::floor((light_color_range - 10) * color.b, 10) + 10);
			a += static_cast<float>(math::floor((light_color_range - 10) * color.a, 10) + 10);

			return *this;
		}

		FORCE_INLINE constexpr color& to_light_color() noexcept{
			r = static_cast<float>(math::floor((light_color_range - 10) * r, 10) + 10);
			g = static_cast<float>(math::floor((light_color_range - 10) * g, 10) + 10);
			b = static_cast<float>(math::floor((light_color_range - 10) * b, 10) + 10);
			a = static_cast<float>(math::floor((light_color_range - 10) * a, 10) + 10);

			return *this;
		}

		constexpr color& set_light(float lumaScl = max_luma_scale) noexcept{
			r *= lumaScl;
			g *= lumaScl;
			b *= lumaScl;
			return *this;
		}

		[[nodiscard]] constexpr color to_light(float lumaScl = max_luma_scale) const noexcept{
			return copy().set_light(lumaScl);
		}

		[[nodiscard]] FORCE_INLINE constexpr color to_light_color_copy() const noexcept{
			color ret{*this};

			return ret.to_light_color();
		}
		[[nodiscard]] FORCE_INLINE constexpr color to_light_color_copy(const bool cond) const noexcept{
			if(cond){
				color ret{*this};

				return ret.to_light_color();
			}else{
				return *this;
			}
		}

		FORCE_INLINE friend constexpr std::size_t hash_value(const color& obj){
			return obj.hash_value();
		}

		FORCE_INLINE friend std::ostream& operator<<(std::ostream& os, const color& obj){
			return os << obj.to_string();
		}

		constexpr friend bool operator==(const color& lhs, const color& rhs) noexcept = default;

		static auto string_to_rgba(const std::string_view hexStr) noexcept{
			std::array<std::uint8_t, 4> rgba{};
			for(const auto& [index, v1] : hexStr
			    | std::views::slide(2)
			    | std::views::stride(2)
			    | std::views::take(4)
			    | std::views::enumerate){
				std::from_chars(v1.data(), v1.data() + v1.size(), rgba[index], 16);
			}

			if(hexStr.size() <= 6){
				rgba[3] = std::numeric_limits<std::uint8_t>::max();
			}

			return rgba;
		}

		static rgba8_bits string_to_rgba_bits(const std::string_view hexStr) noexcept{
			auto value = std::bit_cast<rgba8_bits>(string_to_rgba(hexStr));
			if constexpr(std::endian::native == std::endian::little){
				value = std::byteswap(value);
			}

			return value;
		}

		static color from_string(const std::string_view hexStr){
			const auto rgba = string_to_rgba(hexStr);

			color color{
					static_cast<float>(rgba[0]) / static_cast<float>(std::numeric_limits<std::uint8_t>::max()),
					static_cast<float>(rgba[1]) / static_cast<float>(std::numeric_limits<std::uint8_t>::max()),
					static_cast<float>(rgba[2]) / static_cast<float>(std::numeric_limits<std::uint8_t>::max()),
					static_cast<float>(rgba[3]) / static_cast<float>(std::numeric_limits<std::uint8_t>::max()),
				};

			return color;
		}

		FORCE_INLINE static constexpr rgba8_bits rgb888(const float r, const float g, const float b) noexcept{
			return static_cast<rgba8_bits>(r * max_val) << 16 | static_cast<rgba8_bits>(g * max_val) << 8 | static_cast<
				rgba8_bits>(b * max_val);
		}

		FORCE_INLINE static constexpr rgba8_bits rgba8888(const float r, const float g, const float b,
		                                                  const float a) noexcept{
			return
				static_cast<rgba8_bits>(r * max_val) << r_Offset & r_Mask |
				static_cast<rgba8_bits>(g * max_val) << g_Offset & g_Mask |
				static_cast<rgba8_bits>(b * max_val) << b_Offset & b_Mask |
				static_cast<rgba8_bits>(a * max_val) << a_Offset & a_Mask;
		}

		[[nodiscard]] FORCE_INLINE constexpr rgba8_bits to_rgb888() const noexcept{
			return rgb888(r, g, b);
		}

		[[nodiscard]] FORCE_INLINE constexpr rgba8_bits to_rgba8888() const noexcept{
			return rgba8888(r, g, b, a);
		}

		[[nodiscard]] FORCE_INLINE constexpr rgba8_bits to_rgba() const noexcept{
			return to_rgba8888();
		}

		FORCE_INLINE static constexpr color from_rgb888(const rgba8_bits value, float a = 1) noexcept{
			color c{};
			c.r = static_cast<float>(value >> 16 & max_val) / max_val_f;
			c.g = static_cast<float>(value >> 8 & max_val) / max_val_f;
			c.b = static_cast<float>(value >> 0 & max_val) / max_val_f;
			c.a = a;
			return c;
		}

		FORCE_INLINE static constexpr color from_rgba8888(const rgba8_bits value) noexcept{
			color c{};
			c.r = static_cast<float>(value >> r_Offset & max_val) / max_val_f;
			c.g = static_cast<float>(value >> g_Offset & max_val) / max_val_f;
			c.b = static_cast<float>(value >> b_Offset & max_val) / max_val_f;
			c.a = static_cast<float>(value >> a_Offset & max_val) / max_val_f;
			return c;
		}

		[[nodiscard]] FORCE_INLINE float diff(const color& other) const noexcept{
			const auto lhsv = to_hsv();
			const auto rhsv = other.to_hsv();
			return math::abs(lhsv[0] - rhsv[0]) / 360 + math::abs(lhsv[1] - rhsv[1]) + math::abs(lhsv[2] - rhsv[2]);
		}

		FORCE_INLINE constexpr color& set(const color& color) noexcept{
			return this->operator=(color);
		}

		//TODO operator overload?

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& mul(const color& color) noexcept{
			this->r *= color.r;
			this->g *= color.g;
			this->b *= color.b;
			this->a *= color.a;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& mul_rgb(const float value) noexcept{
			this->r *= value;
			this->g *= value;
			this->b *= value;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& mul_rgba(const float value) noexcept{
			this->r *= value;
			this->g *= value;
			this->b *= value;
			this->a *= value;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& add(const color& color) noexcept{
			this->r += color.r;
			this->g += color.g;
			this->b += color.b;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& sub(const color& color) noexcept{
			this->r -= color.r;
			this->g -= color.g;
			this->b -= color.b;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& set(const float tr, const float tg, const float tb, const float ta) noexcept{
			this->r = tr;
			this->g = tg;
			this->b = tb;
			this->a = ta;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& set(const float tr, const float tg, const float tb) noexcept{
			this->r = tr;
			this->g = tg;
			this->b = tb;
			return clampCond<doClamp>();
		}

		FORCE_INLINE constexpr color& set(const rgba8_bits rgba) noexcept{
			return *this = from_rgba8888(rgba);
		}

		[[nodiscard]] FORCE_INLINE constexpr float sum() const noexcept{
			return r + g + b;
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& add(const float tr, const float tg, const float tb, const float ta) noexcept{
			this->r += tr;
			this->g += tg;
			this->b += tb;
			this->a += ta;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& add(const float tr, const float tg, const float tb) noexcept{
			this->r += tr;
			this->g += tg;
			this->b += tb;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& sub(const float tr, const float tg, const float tb, const float ta) noexcept{
			this->r -= tr;
			this->g -= tg;
			this->b -= tb;
			this->a -= ta;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& sub(const float tr, const float tg, const float tb) noexcept{
			this->r -= tr;
			this->g -= tg;
			this->b -= tb;
			return clampCond<doClamp>();
		}

		FORCE_INLINE constexpr color& inv_rgb(){
			r = 1 - r;
			g = 1 - g;
			b = 1 - b;
			return *this;
		}

		FORCE_INLINE constexpr color& setR(const float tr) noexcept{
			this->r -= tr;
			return *this;
		}

		FORCE_INLINE constexpr color& setG(const float tg) noexcept{
			this->g -= tg;
			return *this;
		}

		FORCE_INLINE constexpr color& setB(const float tb) noexcept{
			this->b -= tb;
			return *this;
		}

		FORCE_INLINE constexpr color& set_a(const float ta) noexcept{
			this->a = ta;
			return *this;
		}

		FORCE_INLINE constexpr color& mul_a(const float ta) noexcept{
			this->a *= ta;
			return *this;
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& mul(const float tr, const float tg, const float tb, const float ta) noexcept{
			this->r *= tr;
			this->g *= tg;
			this->b *= tb;
			this->a *= ta;
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& mul(const float val) noexcept{
			this->r *= val;
			this->g *= val;
			this->b *= val;
			this->a *= val;
			return clampCond<doClamp>();
		}

		using vector4::lerp;

		constexpr FORCE_INLINE auto& light_reserved_lerp(const color& target, const float t) noexcept{
			color base_self = *this % 10.f;
			color base_target = target % 10.f;

			color light_self = (*this - base_self) / light_color_range;
			color light_target = (target - base_target) / light_color_range;

			base_self.lerp(base_target, t);
			light_self.lerp(light_target, t);
			return set(base_self).append_light_color(light_self);
		}

		// template <bool doClamp = true>
		// FORCE_INLINE constexpr color& lerp(const color& target, const float t) noexcept{
		// 	math::lerp_inplace(r, target.r, t);
		// 	math::lerp_inplace(g, target.g, t);
		// 	math::lerp_inplace(b, target.b, t);
		// 	math::lerp_inplace(a, target.a, t);
		// 	return clampCond<doClamp>();
		// }

		template <bool doClamp = false>
		FORCE_INLINE constexpr color& lerpRGB(const color& target, const float t) noexcept{
			math::lerp_inplace(r, target.r, t);
			math::lerp_inplace(g, target.g, t);
			math::lerp_inplace(b, target.b, t);
			return clampCond<doClamp>();
		}

		template <bool doClamp = false>
		[[nodiscard]] FORCE_INLINE constexpr color create_lerp(const color& target, const float t) const noexcept{
			color newColor{
					math::lerp(r, target.r, t),
					math::lerp(g, target.g, t),
					math::lerp(b, target.b, t),
					math::lerp(a, target.a, t)
				};

			return newColor.clampCond<doClamp>();
		}

		template <bool doClamp = false>
		FORCE_INLINE constexpr color& lerp(const float tr, const float tg, const float tb, const float ta,
		                                   const float t) noexcept{
			this->r += t * (tr - this->r);
			this->g += t * (tg - this->g);
			this->b += t * (tb - this->b);
			this->a += t * (ta - this->a);
			return clampCond<doClamp>();
		}

		FORCE_INLINE constexpr color& mul_self_alpha() noexcept{
			r *= a;
			g *= a;
			b *= a;
			return *this;
		}

		FORCE_INLINE constexpr color& write(color& to) const noexcept{
			return to.set(*this);
		}

		//TODO independent hsv type?
		using hsv_t = std::array<float, 3>;

		[[nodiscard]] FORCE_INLINE constexpr float hue() const noexcept{
			return to_hsv()[0];
		}

		[[nodiscard]] FORCE_INLINE constexpr float saturation() const noexcept{
			return to_hsv()[1];
		}

		[[nodiscard]] FORCE_INLINE constexpr float value() const noexcept{
			return to_hsv()[2];
		}

		FORCE_INLINE color& from_hue(const float amount) noexcept{
			hsv_t TMP_HSV = to_hsv();
			TMP_HSV[0] = amount;
			from_hsv(TMP_HSV);
			return *this;
		}

		FORCE_INLINE color& from_saturation(const float amount) noexcept{
			hsv_t TMP_HSV = to_hsv();
			TMP_HSV[1] = amount;
			from_hsv(TMP_HSV);
			return *this;
		}

		FORCE_INLINE color& from_value(const float amount) noexcept{
			hsv_t TMP_HSV = to_hsv();
			TMP_HSV[2] = amount;
			from_hsv(TMP_HSV);
			return *this;
		}

		FORCE_INLINE color& shift_hue(const float amount) noexcept{
			hsv_t TMP_HSV = to_hsv();
			TMP_HSV[0] += amount;
			from_hsv(TMP_HSV);
			return *this;
		}

		FORCE_INLINE color& shift_saturation(const float amount) noexcept{
			hsv_t TMP_HSV = to_hsv();
			TMP_HSV[1] += amount;
			from_hsv(TMP_HSV);
			return *this;
		}

		FORCE_INLINE color& shift_value(const float amount) noexcept{
			hsv_t TMP_HSV = to_hsv();
			TMP_HSV[2] += amount;
			from_hsv(TMP_HSV);
			return *this;
		}

		[[nodiscard]] FORCE_INLINE constexpr std::size_t hash_value() const noexcept{
			return to_rgba8888();
		}

		[[nodiscard]] FORCE_INLINE constexpr rgba8_bits abgr() const noexcept{
			return static_cast<rgba8_bits>(255 * a) << 24 | static_cast<rgba8_bits>(255 * b) << 16 | static_cast<
				rgba8_bits>(255 * g) << 8 | static_cast<rgba8_bits>(255 * r);
		}

		[[nodiscard]] FORCE_INLINE std::string to_string() const{
			return std::format("{:02X}{:02X}{:02X}{:02X}", static_cast<rgba8_bits>(max_val * r),
			                   static_cast<rgba8_bits>(max_val * g), static_cast<rgba8_bits>(max_val * b),
			                   static_cast<rgba8_bits>(max_val * a));;
		}

		FORCE_INLINE color& from_hsv(const float h, const float s, const float v) noexcept{
			const float x = std::fmod(h / 60.0f + 6, static_cast<float>(6));
			const int i = static_cast<int>(x);
			const float f = x - static_cast<float>(i);
			const float p = v * (1 - s);
			const float q = v * (1 - s * f);
			const float t = v * (1 - s * (1 - f));
			switch(i){
			case 0 : r = v;
				g = t;
				b = p;
				break;
			case 1 : r = q;
				g = v;
				b = p;
				break;
			case 2 : r = p;
				g = v;
				b = t;
				break;
			case 3 : r = p;
				g = q;
				b = v;
				break;
			case 4 : r = t;
				g = p;
				b = v;
				break;
			default : r = v;
				g = p;
				b = q;
			}

			return clamp();
		}

		FORCE_INLINE color& from_hsv(const hsv_t hsv) noexcept{
			return from_hsv(hsv[0], hsv[1], hsv[2]);
		}

		FORCE_INLINE constexpr color HSVtoRGB(const float h, const float s, const float v, const float alpha) noexcept{
			color c = HSVtoRGB(h, s, v);
			c.a = alpha;
			return c;
		}

		FORCE_INLINE constexpr color HSVtoRGB(const float h, const float s, const float v) noexcept{
			color c{1, 1, 1, 1};
			HSVtoRGB(h, s, v, c);
			return c;
		}

		[[nodiscard]] FORCE_INLINE constexpr hsv_t to_hsv() const noexcept{
			hsv_t hsv = {};

			const float maxV = math::max(math::max(r, g), b);
			const float minV = math::min(math::min(r, g), b);
			if(const float range = maxV - minV; range == 0){
				hsv[0] = 0;
			} else if(maxV == r){
				hsv[0] = std::fmod(60 * (g - b) / range + 360, 360.0f);
			} else if(maxV == g){
				hsv[0] = 60 * (b - r) / range + 120;
			} else{
				hsv[0] = 60 * (r - g) / range + 240;
			}

			if(maxV > 0){
				hsv[1] = 1 - minV / maxV;
			} else{
				hsv[1] = 0;
			}

			hsv[2] = maxV;

			return hsv;
		}

		[[nodiscard]] FORCE_INLINE bool equals(const color& other) const noexcept{
			constexpr float tolerance = 0.5f / max_val_f;
			return
				math::equal(r, other.r, tolerance) &&
				math::equal(g, other.g, tolerance) &&
				math::equal(b, other.b, tolerance) &&
				math::equal(a, other.a, tolerance);
		}

		constexpr color& HSVtoRGB(float h, float s, float v, color& targetColor) noexcept{
			if(h == 360) h = 359;
			h = math::max(0.0f, math::min(360.0f, h));
			s = math::max(0.0f, math::min(100.0f, s));
			v = math::max(0.0f, math::min(100.0f, v));
			s /= 100.0f;
			v /= 100.0f;
			h /= 60.0f;
			const int i = math::floorLEqual(h);
			const float f = h - static_cast<float>(i);
			const float p = v * (1 - s);
			const float q = v * (1 - s * f);
			const float t = v * (1 - s * (1 - f));
			switch(i){
			case 0 : r = v;
				g = t;
				b = p;
				break;
			case 1 : r = q;
				g = v;
				b = p;
				break;
			case 2 : r = p;
				g = v;
				b = t;
				break;
			case 3 : r = p;
				g = q;
				b = v;
				break;
			case 4 : r = t;
				g = p;
				b = v;
				break;
			default : r = v;
				g = p;
				b = q;
			}

			targetColor.set_a(targetColor.a);
			return targetColor;
		}

		template <std::ranges::random_access_range Rng>
			requires (std::ranges::sized_range<Rng> && std::convertible_to<
				const color&, std::ranges::range_value_t<Rng>>)
		[[nodiscard]] static constexpr color from_lerp_span(float s, const Rng& colors) noexcept{
			s = math::clamp(s);
			const std::size_t size = std::ranges::size(colors);
			const std::size_t bound = size - 1;

			const auto boundf = static_cast<float>(bound);

			const color& ca = std::ranges::begin(colors)[static_cast<std::size_t>(s * boundf)];
			auto nextIdx = static_cast<std::size_t>(s * boundf + 1);

			if(nextIdx == size){
				return ca;
			}

			const color& cb = std::ranges::begin(colors)[nextIdx];

			const float n = s * boundf - static_cast<float>(static_cast<int>(s * boundf));
			const float i = 1.0f - n;
			return color{ca.r * i + cb.r * n, ca.g * i + cb.g * n, ca.b * i + cb.b * n, ca.a * i + cb.a * n};
		}


		[[nodiscard]] static constexpr color from_lerp_span(float s, const auto&... colors) noexcept{
			return color::from_lerp_span(s, std::array{colors...});
		}

		constexpr color& lerp_span(const float s, const auto&... colors) noexcept{
			return this->set(color::from_lerp_span(s, colors...));
		}

		template <std::ranges::random_access_range Rng>
			requires (std::ranges::sized_range<Rng> && std::convertible_to<
				const color&, std::ranges::range_value_t<Rng>>)
		constexpr color& lerp_span(const float s, const Rng& colors) noexcept{
			return this->operator=(color::lerp_span(s, colors));
		}

		[[nodiscard]] constexpr color copy() const noexcept{
			return {*this};
		}
	};

	export namespace colors{
		constexpr color white{1, 1, 1, 1};
		constexpr color light_gray{color::from_rgba8888(0xbfbfbfff)};
		constexpr color gray{color::from_rgba8888(0x7f7f7fff)};
		constexpr color dark_gray{color::from_rgba8888(0x3f3f3fff)};
		constexpr color black{0, 0, 0, 1};
		constexpr color clear{0, 0, 0, 0};
		constexpr color black_clear{0, 0, 0, 0};
		constexpr color white_clear{1, 1, 1, 0};
		constexpr color clear_light{white.to_light().set_a(0)};

		constexpr color BLUE{0, 0, 1, 1};
		constexpr color NAVY{0, 0, 0.5f, 1};
		constexpr color ROYAL{color::from_rgba8888(0x4169e1ff)};
		constexpr color SLATE{color::from_rgba8888(0x708090ff)};
		constexpr color SKY{color::from_rgba8888(0x87ceebff)};

		constexpr color aqua{color::from_rgba8888(0x85A2F3ff)};
		constexpr color BLUE_SKY = color::from_lerp_span(0.745f, BLUE, SKY);
		constexpr color AQUA_SKY = color::from_lerp_span(0.5f, aqua, SKY);

		constexpr color CYAN{0, 1, 1, 1};
		constexpr color TEAL{0, 0.5f, 0.5f, 1};

		constexpr color GREEN{color::from_rgba8888(0x00ff00ff)};
		constexpr color pale_green{color::from_rgba8888(0xa1ecabff)};
		constexpr color LIGHT_GREEN{color::from_rgba8888(0X62F06CFF)};
		constexpr color ACID{color::from_rgba8888(0x7fff00ff)};
		constexpr color LIME{color::from_rgba8888(0x32cd32ff)};
		constexpr color FOREST{color::from_rgba8888(0x228b22ff)};
		constexpr color OLIVE{color::from_rgba8888(0x6b8e23ff)};

		constexpr color YELLOW{color::from_rgba8888(0xffff00ff)};
		constexpr color GOLD{color::from_rgba8888(0xffd700ff)};
		constexpr color GOLDENROD{color::from_rgba8888(0xdaa520ff)};
		constexpr color ORANGE{color::from_rgba8888(0xffa500ff)};

		constexpr color BROWN{color::from_rgba8888(0x8b4513ff)};
		constexpr color TAN{color::from_rgba8888(0xd2b48cff)};
		constexpr color BRICK{color::from_rgba8888(0xb22222ff)};

		constexpr color RED{color::from_rgba8888(0xff0000ff)};
		constexpr color red_dusted{color::from_rgba8888(0xDE6663ff)};
		constexpr color SCARLET{color::from_rgba8888(0xff341cff)};
		constexpr color CRIMSON{color::from_rgba8888(0xdc143cff)};
		constexpr color CORAL{color::from_rgba8888(0xff7f50ff)};
		constexpr color SALMON{color::from_rgba8888(0xfa8072ff)};
		constexpr color PINK{color::from_rgba8888(0xff69b4ff)};
		constexpr color MAGENTA{1, 0, 1, 1};

		constexpr color PURPLE{color::from_rgba8888(0xa020f0ff)};
		constexpr color VIOLET{color::from_rgba8888(0xee82eeff)};
		constexpr color MAROON{color::from_rgba8888(0xb03060ff)};

		constexpr color ENERGY{color::from_rgba8888(0XF0C743FF)};
		constexpr color AMMUNITION{color::from_rgba8888(0XF0A24BFF)};
	}
}

export
template <>
struct ::std::hash<mo_yanxi::graphic::color>{
	size_t operator()(const mo_yanxi::graphic::color& obj) const noexcept{
		return obj.hash_value();
	}
};

export
template <>
struct ::std::formatter<mo_yanxi::graphic::color>{
	bool haveAlpha{false};
	bool haveWrapper{false};

	constexpr auto parse(std::format_parse_context& context){
		auto it = context.begin();
		if(it == context.end()) return it;

		if(*it == '[' || *it == ']'){
			haveWrapper = true;
			++it;
		}

		if(*it == 'a'){
			haveAlpha = true;
			++it;
		}
		if(it != context.end() && *it != '}') throw std::format_error("Invalid format");

		return it;
	}

	auto format(const mo_yanxi::graphic::color& c, auto& context) const{
		using mo_yanxi::graphic::color;
		using ColorBits = color::rgba8_bits;
		if(haveAlpha){
			if(haveWrapper){
				return std::format_to(context.out(), "[{:02X}{:02X}{:02X}{:02X}]",
				                      static_cast<ColorBits>(color::max_val * c.r),
				                      static_cast<ColorBits>(color::max_val * c.g),
				                      static_cast<ColorBits>(color::max_val * c.b),
				                      static_cast<ColorBits>(color::max_val * c.a));
			} else{
				return std::format_to(context.out(), "{:02X}{:02X}{:02X}{:02X}",
				                      static_cast<ColorBits>(color::max_val * c.r),
				                      static_cast<ColorBits>(color::max_val * c.g),
				                      static_cast<ColorBits>(color::max_val * c.b),
				                      static_cast<ColorBits>(color::max_val * c.a));
			}
		} else{
			if(haveWrapper){
				return std::format_to(context.out(), "[{:02X}{:02X}{:02X}]",
				                      static_cast<ColorBits>(color::max_val * c.r),
				                      static_cast<ColorBits>(color::max_val * c.g),
				                      static_cast<ColorBits>(color::max_val * c.b));
			} else{
				return std::format_to(context.out(), "{:02X}{:02X}{:02X}",
				                      static_cast<ColorBits>(color::max_val * c.r),
				                      static_cast<ColorBits>(color::max_val * c.g),
				                      static_cast<ColorBits>(color::max_val * c.b));
			}
		}
	}
};
