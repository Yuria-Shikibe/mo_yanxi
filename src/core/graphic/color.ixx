module;

#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.color;


import std;
import mo_yanxi.math;
import mo_yanxi.math.vector4;
import mo_yanxi.math.vector4;

namespace mo_yanxi{
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

		constexpr color() noexcept : color(1, 1, 1, 1){
		}

		constexpr explicit color(const rgba8_bits rgba8888V) noexcept{
			from_rgba8888(rgba8888V);
		}

		constexpr color(const float r, const float g, const float b, const float a) noexcept
			: vector4<float>(r, g, b, a){
		}

		constexpr color(const float r, const float g, const float b) noexcept : color(r, g, b, 1){
		}

	private:
		FORCE_INLINE constexpr color& clamp() noexcept{
			r = math::clamp(r);
			g = math::clamp(g);
			b = math::clamp(b);
			a = math::clamp(a);

			return *this;
		}

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
			r += static_cast<float>(math::floor(light_color_range * color.r, 10));
			g += static_cast<float>(math::floor(light_color_range * color.g, 10));
			b += static_cast<float>(math::floor(light_color_range * color.b, 10));
			a += static_cast<float>(math::floor(light_color_range * color.a, 10));

			return *this;
		}

		FORCE_INLINE constexpr color& to_light_color() noexcept{
			r = static_cast<float>(math::floor(light_color_range * r, 10));
			g = static_cast<float>(math::floor(light_color_range * g, 10));
			b = static_cast<float>(math::floor(light_color_range * b, 10));
			a = static_cast<float>(math::floor(light_color_range * a, 10));

			return *this;
		}

		[[nodiscard]] FORCE_INLINE constexpr color to_light_color_copy() const noexcept{
			color ret{*this};
			ret.r = static_cast<float>(math::floor(light_color_range * r, 10));
			ret.g = static_cast<float>(math::floor(light_color_range * g, 10));
			ret.b = static_cast<float>(math::floor(light_color_range * b, 10));
			ret.a = static_cast<float>(math::floor(light_color_range * a, 10));

			return ret;
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

		FORCE_INLINE constexpr color& from_rgb888(const rgba8_bits value) noexcept{
			r = static_cast<float>(value >> 16 & max_val) / max_val_f;
			g = static_cast<float>(value >> 8 & max_val) / max_val_f;
			b = static_cast<float>(value >> 0 & max_val) / max_val_f;
			return *this;
		}

		FORCE_INLINE constexpr color& from_rgba8888(const rgba8_bits value) noexcept{
			r = static_cast<float>(value >> r_Offset & max_val) / max_val_f;
			g = static_cast<float>(value >> g_Offset & max_val) / max_val_f;
			b = static_cast<float>(value >> b_Offset & max_val) / max_val_f;
			a = static_cast<float>(value >> a_Offset & max_val) / max_val_f;
			return *this;
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
			return from_rgba8888(rgba);
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

		FORCE_INLINE constexpr color& setA(const float ta) noexcept{
			this->a = ta;
			return *this;
		}

		FORCE_INLINE constexpr color& mulA(const float ta) noexcept{
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

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& lerp(const color& target, const float t) noexcept{
			math::lerp_inplace(r, target.r, t);
			math::lerp_inplace(g, target.g, t);
			math::lerp_inplace(b, target.b, t);
			math::lerp_inplace(a, target.a, t);
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		FORCE_INLINE constexpr color& lerpRGB(const color& target, const float t) noexcept{
			math::lerp_inplace(r, target.r, t);
			math::lerp_inplace(g, target.g, t);
			math::lerp_inplace(b, target.b, t);
			return clampCond<doClamp>();
		}

		template <bool doClamp = true>
		[[nodiscard]] FORCE_INLINE constexpr color create_lerp(const color& target, const float t) const noexcept{
			color newColor{
					math::lerp(r, target.r, t),
					math::lerp(g, target.g, t),
					math::lerp(b, target.b, t),
					math::lerp(a, target.a, t)
				};

			return newColor.clampCond<doClamp>();
		}

		template <bool doClamp = true>
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

			targetColor.setA(targetColor.a);
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
		constexpr color WHITE{1, 1, 1, 1};
		constexpr color LIGHT_GRAY{0xbfbfbfff};
		constexpr color GRAY{0x7f7f7fff};
		constexpr color DARK_GRAY{0x3f3f3fff};
		constexpr color BLACK{0, 0, 0, 1};
		constexpr color CLEAR{0, 0, 0, 0};

		constexpr color BLUE{0, 0, 1, 1};
		constexpr color NAVY{0, 0, 0.5f, 1};
		constexpr color ROYAL{0x4169e1ff};
		constexpr color SLATE{0x708090ff};
		constexpr color SKY{0x87ceebff};

		constexpr color AQUA{0x85A2F3ff};
		constexpr color BLUE_SKY = color::from_lerp_span(0.745f, BLUE, SKY);
		constexpr color AQUA_SKY = color::from_lerp_span(0.5f, AQUA, SKY);

		constexpr color CYAN{0, 1, 1, 1};
		constexpr color TEAL{0, 0.5f, 0.5f, 1};

		constexpr color GREEN{0x00ff00ff};
		constexpr color PALE_GREEN{0xa1ecabff};
		constexpr color LIGHT_GREEN{0X62F06CFF};
		constexpr color ACID{0x7fff00ff};
		constexpr color LIME{0x32cd32ff};
		constexpr color FOREST{0x228b22ff};
		constexpr color OLIVE{0x6b8e23ff};

		constexpr color YELLOW{0xffff00ff};
		constexpr color GOLD{0xffd700ff};
		constexpr color GOLDENROD{0xdaa520ff};
		constexpr color ORANGE{0xffa500ff};

		constexpr color BROWN{0x8b4513ff};
		constexpr color TAN{0xd2b48cff};
		constexpr color BRICK{0xb22222ff};

		constexpr color RED{0xff0000ff};
		constexpr color RED_DUSK{0xDE6663ff};
		constexpr color SCARLET{0xff341cff};
		constexpr color CRIMSON{0xdc143cff};
		constexpr color CORAL{0xff7f50ff};
		constexpr color SALMON{0xfa8072ff};
		constexpr color PINK{0xff69b4ff};
		constexpr color MAGENTA{1, 0, 1, 1};

		constexpr color PURPLE{0xa020f0ff};
		constexpr color VIOLET{0xee82eeff};
		constexpr color MAROON{0xb03060ff};

		constexpr color ENERGY{0XF0C743FF};
		constexpr color AMMUNITION{0XF0A24BFF};
	}
}

export
template <>
struct ::std::hash<mo_yanxi::color>{
	size_t operator()(const mo_yanxi::color& obj) const noexcept{
		return obj.hash_value();
	}
};

export
template <>
struct ::std::formatter<mo_yanxi::color>{
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

	auto format(const mo_yanxi::color& c, auto& context) const{
		using mo_yanxi::color;
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
