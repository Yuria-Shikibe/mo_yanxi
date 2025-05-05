module;

#if DEBUG_CHECK


#define FT_CONFIG_OPTION_ERROR_STRINGS
#endif

#include <cassert>
#include <ft2build.h>
#include <freetype/freetype.h>

//TODO hide msdf impl
extern "C++"{
	#include <msdfgen/msdfgen.h>
	#include <msdfgen/msdfgen-ext.h>
}

export module mo_yanxi.font;
import std;

import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.vector2;

import mo_yanxi.graphic.bitmap;
import mo_yanxi.graphic.msdf;

import mo_yanxi.math;

import mo_yanxi.heterogeneous;
import mo_yanxi.handle_wrapper;
import mo_yanxi.open_addr_hash_map;
import mo_yanxi.referenced_ptr;

namespace mo_yanxi::font{
	export constexpr inline math::vec2 font_draw_expand{graphic::msdf::sdf_image_boarder, graphic::msdf::sdf_image_boarder};
	// export constexpr inline math::vec2 font_draw_expand{8, 8};

	void check(FT_Error error);

	export struct library : exclusive_handle<FT_Library>{
		exclusive_handle_member<msdfgen::FreetypeHandle*> msdfHdl{};

		[[nodiscard]] library(){
			check(FT_Init_FreeType(as_data()));
			msdfHdl = msdfgen::initializeFreetype();
		}

		~library(){
			if(handle)check(FT_Done_FreeType(handle));
			if(msdfHdl)msdfgen::deinitializeFreetype(msdfHdl);
		}

		library(const library& other) = delete;
		library(library&& other) noexcept = default;
		library& operator=(const library& other) = delete;
		library& operator=(library&& other) noexcept = default;
	};

	export inline const library global_free_type_lib{}; // NOLINT(*-err58-cpp)

	export using char_code = char32_t;
	export using glyph_size_type = math::vector2<std::uint16_t>;
	export using glyph_raw_metrics = FT_Glyph_Metrics;

	export using FontFaceID = std::uint32_t;

	export
	bool is_space(char_code code) noexcept{
		return code == U'\0' || code <= std::numeric_limits<signed char>::max() && std::isspace(code);
	}

	export
	bool is_unicode_separator(char_code c) noexcept{

		// ASCII控制字符（换行/回车/制表符等）
		if (c < 0x80 && std::iscntrl(static_cast<unsigned char>(c))) {
			return true;
		}

		// 常见ASCII分隔符
		if (c < 0x80 && (std::ispunct(static_cast<unsigned char>(c)) || c == ' ')) {
			return true;
		}

		// 全角标点（FF00-FFEF区块）
		if ((c >= 0xFF01 && c <= 0xFF0F) ||  // ！＂＃＄％＆＇（）＊＋，－．／
			(c >= 0xFF1A && c <= 0xFF20) ||  //：；＜＝＞？＠
			(c >= 0xFF3B && c <= 0xFF40) ||  //；＜＝＞？＠［＼］＾＿
			(c >= 0xFF5B && c <= 0xFF65)) {  //｛｜｝～｟...等
			return true;
			}

		// 中文标点（3000-303F区块）
		if ((c >= 0x3000 && c <= 0x303F)) {  // 包含　、。〃〄等
			return true;
		}

		return false;
	}


	export
	template <typename T>
	constexpr T get_snapped_size(const T len) noexcept{
		// return len;

		if(len == 0)return 0;
		if(len <= static_cast<T>(256))return 64;
		return 128;
	}


	export struct glyph_identity{
		char_code code{};
		glyph_size_type size{};

		constexpr friend bool operator==(const glyph_identity&, const glyph_identity&) noexcept = default;
	};

	export
	template <std::floating_point T = float>
	constexpr T normalize_len(const FT_Pos pos) noexcept {
		return std::ldexp(static_cast<T>(pos), -6); // NOLINT(*-narrowing-conversions)
	}

	export
	template <std::floating_point T = float>
	constexpr T normalize_len_1616(const FT_Pos pos) noexcept {
		return static_cast<T>(pos) / static_cast<T>(65536.); // NOLINT(*-narrowing-conversions)
	}

	export struct glyph_metrics{
		math::vec2 size{};
		math::vec2 horiBearing{};
		math::vec2 vertBearing{};
		math::vec2 advance{};

		[[nodiscard]] constexpr glyph_metrics() = default;

		[[nodiscard]] constexpr explicit(false) glyph_metrics(const glyph_raw_metrics& metrics){
			size.x = normalize_len<float>(metrics.width);
			size.y = normalize_len<float>(metrics.height);
			horiBearing.x = normalize_len<float>(metrics.horiBearingX);
			horiBearing.y = normalize_len<float>(metrics.horiBearingY);
			advance.x = normalize_len<float>(metrics.horiAdvance);
			vertBearing.x = normalize_len<float>(metrics.vertBearingX);
			vertBearing.y = normalize_len<float>(metrics.vertBearingY);
			advance.y = normalize_len<float>(metrics.vertAdvance);

			if(metrics.height == 0 && metrics.horiBearingY == 0){
				size.y = horiBearing.y = vertBearing.y;
			}
		}

		[[nodiscard]] constexpr float ascender() const noexcept{
			return horiBearing.y;
		}

		[[nodiscard]] constexpr float descender() const noexcept{
			return size.y - horiBearing.y;
		}

		/**
		 * @param pos Pen position
		 * @param scale
		 * @return [v00, v11]
		 */
		[[nodiscard]] math::frect place_to(const math::vec2 pos, const math::vec2 scale) const{
			math::vec2 src = pos;
			math::vec2 end = pos;
			src.add(horiBearing.x, -horiBearing.y * scale.y);
			end.add(horiBearing.x + size.x * scale.x, descender() * scale.y);

			return {tags::unchecked, src, end};
		}
	};

	graphic::bitmap to_pixmap(const FT_Bitmap& map);

	export constexpr auto ASCII_CHARS = []() constexpr {
		constexpr std::size_t Size = '~' - ' ' + 1;
		const auto targetChars = std::ranges::views::iota(' ', '~' + 1) | std::ranges::to<std::vector<char_code>>();
		std::array<char_code, Size> charCodes{};

		for(auto [idx, charCode] : targetChars | std::views::enumerate){
			charCodes[idx] = charCode;
		}

		return charCodes;
	}();

	struct font_face_handle : exclusive_handle<FT_Face>{
		exclusive_handle_member<msdfgen::FontHandle*> msdfHdl{};

		[[nodiscard]] font_face_handle() = default;

		~font_face_handle(){
			if(handle)check(FT_Done_Face(handle));
			if(msdfHdl)msdfgen::destroyFont(msdfHdl);
		}

		font_face_handle(const font_face_handle& other) = delete;
		font_face_handle(font_face_handle&& other) noexcept = default;
		font_face_handle& operator=(const font_face_handle& other) = delete;
		font_face_handle& operator=(font_face_handle&& other) noexcept = default;

		explicit font_face_handle(const char* path, const FT_Long index = 0){
			check(FT_New_Face(global_free_type_lib, path, index, &handle));
			msdfHdl = msdfgen::loadFont(global_free_type_lib.msdfHdl, path);
		}


		[[nodiscard]] FT_UInt index_of(const char_code code) const{
			return FT_Get_Char_Index(handle, code);
		}

		[[nodiscard]] FT_Error load_glyph(const FT_UInt index, const FT_Int32 loadFlags) const{
			return FT_Load_Glyph(handle, index, loadFlags);
		}

		[[nodiscard]] FT_Error load_char(const FT_ULong code, const FT_Int32 loadFlags) const{
			return FT_Load_Char(handle, code, loadFlags);
		}

		void set_size(const unsigned w, const unsigned h = 0u) const{
			if((w && handle->size->metrics.x_ppem == w) && (h && handle->size->metrics.y_ppem == h))return;
			FT_Set_Pixel_Sizes(handle, w, h);
		}

		void set_size(const math::usize2 size2) const{
			set_size( size2.x, size2.y);
		}

		[[nodiscard]] auto getHeight() const{
			return handle->height;
		}

		[[nodiscard]] std::expected<FT_GlyphSlot, FT_Error> load_and_get(
			const char_code index,
			const FT_Int32 loadFlags = FT_LOAD_DEFAULT) const{
			auto glyph_index = index_of(index); // Check if glyph index is zero, which means the character is not in the font
			if (index != 0 && glyph_index == 0){
				return std::expected<FT_GlyphSlot, FT_Error>{std::unexpect, FT_Err_Invalid_Character_Code};
			}

			if(auto error = load_glyph(glyph_index, loadFlags)){
				return std::expected<FT_GlyphSlot, FT_Error>{std::unexpect, error};
			}

			return std::expected<FT_GlyphSlot, FT_Error>{std::in_place, handle->glyph};
		}

		[[nodiscard]] FT_GlyphSlot load_and_get_guaranteed(const char_code index, const FT_Int32 loadFlags) const{
			if(const auto error = load_char(index, loadFlags)){
				check(error);
			}

			return handle->glyph;
		}

		[[nodiscard]] std::string_view get_family_name() const{
			return {handle->family_name};
		}

		[[nodiscard]] std::string_view get_style_name() const{
			return {handle->style_name};
		}

		[[nodiscard]] std::string get_fullname() const{
			return std::format("{}.{}", get_family_name(), get_style_name());
		}

		[[nodiscard]] auto get_face_index() const{
			return handle->face_index;
		}
	};



	export
	graphic::bitmap render_sdf(const FT_GlyphSlot shot){
		// FT_Render_Glyph(shot, FT_RENDER_MODE_NORMAL);
		FT_Render_Glyph(shot, FT_RENDER_MODE_SDF);

		return to_pixmap(shot->bitmap);
	}

	export struct glyph_wrap : referenced_object<false>{
		char_code code{};
		font_face_handle* face{};

		[[nodiscard]] glyph_wrap() = default;

		[[nodiscard]] explicit glyph_wrap(
			const char_code code,
			font_face_handle& face) :
			code{code},
			face(&face){}

		[[nodiscard]] graphic::msdf::msdf_glyph_generator get_generator(const unsigned w, const unsigned h) const noexcept{
			face->set_size(w, h);
			return graphic::msdf::msdf_glyph_generator{
				face->msdfHdl,
				(*face)->size->metrics.x_ppem, (*face)->size->metrics.y_ppem
			};
		}

		[[nodiscard]] math::usize2 get_extent() const noexcept{
			(void)face->load_and_get(code, FT_LOAD_DEFAULT);
			return {(*face)->glyph->bitmap.width + graphic::msdf::sdf_image_boarder * 2, (*face)->glyph->bitmap.rows + graphic::msdf::sdf_image_boarder * 2};
		}
		// [[nodiscard]] math::usize2 extent() const noexcept{
		// 	return {generator.font_w, generator.font_h};
		// }
		//
		// auto crop() const noexcept{
		// 	return generator.crop(code);
		// }
		//
		// [[nodiscard]] explicit bitmap_glyph(const char_code code, const FT_Glyph_Metrics& metrics, math::vec2 scale) :
		// 	code{code},
		// 	metrics{metrics}{
		// 	this->metrics.width *= scale.x;
		// 	this->metrics.height *= scale.y;
		// 	this->metrics.horiBearingX *= scale.x;
		// 	this->metrics.vertBearingX *= scale.x;
		// 	this->metrics.horiBearingY *= scale.y;
		// 	this->metrics.vertBearingY *= scale.y;
		// 	this->metrics.horiAdvance *= scale.x;
		// 	this->metrics.vertAdvance *= scale.y;
		// }
	};

	export struct font_face{

	private:
		font_face_handle face{};

	public:
		//TODO english char replacement ?
		font_face* fallback{};


		[[nodiscard]] font_face() = default;

		[[nodiscard]] explicit font_face(const char* fontPath)
			: face{fontPath}{}

		[[nodiscard]] explicit font_face(const std::string_view fontPath)
			: face{fontPath.data()}{}

		[[nodiscard]] glyph_wrap obtain(const char_code code, const glyph_size_type size) noexcept {
			assert((size.x != 0 || size.y != 0) && "must at least one none zero");

			face.set_size(size.x, size.y);
			if(const auto shot = face.load_and_get(code, FT_LOAD_DEFAULT)){
				if(shot.value()->bitmap.width * shot.value()->bitmap.rows != 0){
					// graphic::bitmap bitmap =
					// 	graphic::msdf::load_glyph(
					// 		face.msdfHdl, code,
					// 		shot.value()->bitmap.width, shot.value()->bitmap.rows,
					// 		shot.value()->face->size->metrics.x_ppem, shot.value()->face->size->metrics.y_ppem,
					// 		get_snapped_range(std::hypot(shot.value()->face->size->metrics.x_ppem, shot.value()->face->size->metrics.y_ppem))
					// 	);

					// auto map = render_sdf(shot.value());
					return glyph_wrap{code, face};
				}


			}

			if(fallback){
				assert(fallback != this);
				return fallback->obtain(code, size);
			}

			return glyph_wrap{code, face};
		}

		float get_line_spacing(const math::usize2 sz) const{
			face.set_size(sz);
			return normalize_len(face->size->metrics.height);
		}

		[[nodiscard]] glyph_wrap obtain(const char_code code, const glyph_size_type::value_type size) noexcept {
			return obtain(code, {size, 0});
		}

		[[nodiscard]] glyph_wrap obtain_snapped(const char_code code, const glyph_size_type::value_type size) noexcept {
			return obtain(code, get_snapped_size(size));
		}

		[[nodiscard]] std::string format(const char_code code, const glyph_size_type size) const{
			return std::format("{}.{}.U{:#0X}[{},{}]", face.get_family_name(), face.get_style_name(), reinterpret_cast<const int&>(code), size.x, size.y);
		}

		// [[nodiscard]] auto dump_names(){
		// 	return glyphs | std::views::values | std::views::join | std::views::transform(
		// 		[this](decltype(glyphs)::mapped_type::value_type& pair){
		// 		return std::make_pair(format(pair.second.code, pair.first), std::ref(pair.second));
		// 	});
		// }
		//
		// void clean() noexcept{
		// 	for (decltype(glyphs)::reference glyph : glyphs){
		// 		std::erase_if(glyph.second, [](std::remove_cvref_t<decltype(glyph)>::second_type::const_reference g){
		// 			return g.second.get_ref_count() == 0;
		// 		});
		// 	}
		//
		// 	std::erase_if(glyphs, [](decltype(glyphs)::const_reference g){
		// 		return g.second.size() == 0;
		// 	});
		// }

		font_face(const font_face& other) = delete;

		font_face(font_face&& other) noexcept = default;

		font_face& operator=(const font_face& other) = delete;

		font_face& operator=(font_face&& other) noexcept = default;
	};

	// export struct FontGlyphLoader{
	// 	font_face_storage& storage;
	// 	glyph_size_type size{};
	// 	std::vector<std::vector<char_code>> sections{};
	//
	// 	void load() const{
	// 		for (const auto code : sections | std::views::join){
	// 			// storage.tryLoad(code, size);
	// 		}
	// 	}
	// };
}

export template <>
struct std::hash<mo_yanxi::font::glyph_identity>{ // NOLINT(*-dcl58-cpp)
	constexpr std::size_t operator()(const mo_yanxi::font::glyph_identity& key) const noexcept{
		return std::bit_cast<std::size_t>(key);
	}
};

// BITMASK_OPS(export, mo_yanxi::font::FontFlag);
// BITMASK_OPS(export, mo_yanxi::font::StyleFlag);

module : private;


mo_yanxi::graphic::bitmap mo_yanxi::font::to_pixmap(const FT_Bitmap& map) {
	graphic::bitmap pixmap{map.width, map.rows};
	auto* const data = pixmap.data();
	const auto size = map.width * map.rows;
	std::memset(data, 0xff, pixmap.size_bytes());
	for(unsigned i = 0; i < size; ++i) {
		data[i].r = map.buffer[i];
		data[i].g = map.buffer[i];
		data[i].b = map.buffer[i];
	}
	return pixmap;
}

void mo_yanxi::font::check(FT_Error error){
	if(!error)return;

#if DEBUG_CHECK
	const char* err = FT_Error_String(error);
	std::println(std::cerr, "[Freetype] Error {}: {}", error, err);
#else
	std::println(std::cerr, "[Freetype] Error {}", error);
#endif

	throw std::runtime_error("Freetype Failed");
}

