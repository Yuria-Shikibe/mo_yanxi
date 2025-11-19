module;

#define MSDFGEN_USE_CPP11
#include <msdfgen/msdfgen.h>
#include <msdfgen/msdfgen-ext.h>

export module mo_yanxi.graphic.msdf;

export import mo_yanxi.graphic.bitmap;

import mo_yanxi.math;
import std;

//


namespace mo_yanxi::graphic::msdf{
export constexpr inline int sdf_image_boarder = 4;
export constexpr inline double sdf_image_range = 1.5f;

export msdfgen::FreetypeHandle* HACK_get_ft_library_from(const void* address_of_ft_lib) noexcept{
	return static_cast<msdfgen::FreetypeHandle*>(const_cast<void*>(address_of_ft_lib));
}


export msdfgen::FontHandle* HACK_get_face_from(void* address_of_ft_face) noexcept{
	return static_cast<msdfgen::FontHandle*>(address_of_ft_face);
}


void write_to_bitmap(bitmap& bitmap, const msdfgen::Bitmap<float, 3>& region){
	for(unsigned y = 0; y < bitmap.height(); ++y){
		for(unsigned x = 0; x < bitmap.width(); ++x){
			auto& bit = bitmap[x, y];
			bit.r = math::round<std::uint8_t>(
				region(x, y)[0] * static_cast<float>(std::numeric_limits<std::uint8_t>::max()));
			bit.g = math::round<std::uint8_t>(
				region(x, y)[1] * static_cast<float>(std::numeric_limits<std::uint8_t>::max()));
			bit.b = math::round<std::uint8_t>(
				region(x, y)[2] * static_cast<float>(std::numeric_limits<std::uint8_t>::max()));
			bit.a = std::numeric_limits<std::uint8_t>::max();
		}
	}
}

void write_to_bitmap(bitmap& bitmap, const msdfgen::Bitmap<float, 1>& region){
	for(unsigned y = 0; y < bitmap.height(); ++y){
		for(unsigned x = 0; x < bitmap.width(); ++x){
			auto& bit = bitmap[x, y];
			bit.r = bit.g = bit.b = math::round<std::uint8_t>(
				region(x, y)[0] * static_cast<float>(std::numeric_limits<std::uint8_t>::max()));
			bit.a = std::numeric_limits<std::uint8_t>::max();
		}
	}
}

// export
// [[nodiscard]] bitmap load_svg(const char* path, unsigned w, unsigned h, double range = 2.);

struct svg_info{
	msdfgen::Shape shape{};
	math::vec2 size{};
};

export
[[nodiscard]] bitmap load_shape(
	const svg_info& shape,
	unsigned w, unsigned h,
	double range = sdf_image_range,
	int boarder = sdf_image_boarder);


export
[[nodiscard]] bitmap load_glyph(
	msdfgen::FontHandle* face,
	msdfgen::unicode_t code,
	unsigned target_w,
	unsigned target_h,
	int boarder,
	double font_w,
	double font_h,
	double range = sdf_image_range
);

export
[[nodiscard]] svg_info svg_to_shape(const char* path);


export
struct msdf_generator{
private:
	svg_info shape{};

public:
	double range = sdf_image_range;
	int boarder = sdf_image_boarder;

	[[nodiscard]] msdf_generator() = default;

	[[nodiscard]] msdf_generator(svg_info&& shape, double range = sdf_image_range, int boarder = sdf_image_boarder);

	[[nodiscard]] msdf_generator(const char* path, double range = sdf_image_range, int boarder = sdf_image_boarder);

	graphic::bitmap operator ()(const unsigned w, const unsigned h, const unsigned mip_lv) const{
		auto scl = 1u << mip_lv;
		auto b = boarder / scl;
		if(b * 2 >= w || b * 2 >= h){
			b = 0;
		}
		return load_shape(shape, w - b * 2, h - b * 2, math::max(range / static_cast<double>(scl), 0.25), b);
	}

	graphic::bitmap operator ()(const unsigned w, const unsigned h) const{
		return load_shape(shape, w, h, range, boarder);
	}
};

struct msdf_glyph_generator_base{
	msdfgen::FontHandle* face{};
	unsigned font_w{};
	unsigned font_h{};
	double range = 0.4;
	unsigned boarder = sdf_image_boarder;
};

export
struct msdf_glyph_generator_crop : msdf_glyph_generator_base{
	msdfgen::unicode_t code{};

	[[nodiscard]] msdf_glyph_generator_crop(const msdf_glyph_generator_base& base, msdfgen::unicode_t code)
	: msdf_glyph_generator_base{base}, code(code){
	}

	bitmap operator()(const unsigned w, const unsigned h, const unsigned mip_lv) const{
		auto scl = 1 << mip_lv;
		auto b = boarder / scl;
		if(b * 2 >= w || b * 2 >= h){
			b = 0;
		}
		return load_glyph(face, code, w - b * 2, h - b * 2, b,
			static_cast<double>(font_w) / static_cast<double>(scl),
			static_cast<double>(font_h) / static_cast<double>(scl), range);
	};
};

export
struct msdf_glyph_generator : msdf_glyph_generator_base{
	[[nodiscard]] msdf_glyph_generator_crop crop(msdfgen::unicode_t code) const noexcept{
		return {*this, code};
	}
};

void add_contour(msdfgen::Shape& shape, double size, double radius, double k, double margin = 0.f,
	msdfgen::Vector2 offset = {}){
	using namespace msdfgen;

	// 创建外轮廓
	Contour& outerContour = shape.addContour();

	Point2 bl_r{radius + margin, margin};
	Point2 br_l{size - radius - margin, margin};
	Point2 br_t{size - margin, margin + radius};
	Point2 tr_b{size - margin, size - radius - margin};
	Point2 tr_l{size - margin - radius, size - margin};
	Point2 tl_r{radius + margin, size - margin};
	Point2 tl_b{margin, size - margin - radius};
	Point2 bl_t{margin, margin + radius};

	bl_r += offset;
	br_l += offset;
	br_t += offset;
	tr_b += offset;
	tr_l += offset;
	tl_r += offset;
	tl_b += offset;
	bl_t += offset;

	Vector2 handle{k * radius, k * radius};

	// 外轮廓路径（顺时针）
	outerContour.addEdge(new LinearSegment(bl_r, br_l));
	outerContour.addEdge(new CubicSegment(
		br_l,
		br_l + handle * Vector2{1, 0},
		br_t + handle * Vector2{0, -1},
		br_t
	));
	outerContour.addEdge(new LinearSegment(br_t, tr_b));
	outerContour.addEdge(new CubicSegment(
		tr_b,
		tr_b + handle * Vector2{0, 1},
		tr_l + handle * Vector2{1, 0},
		tr_l
	));
	outerContour.addEdge(new LinearSegment(tr_l, tl_r));
	outerContour.addEdge(new CubicSegment(
		tl_r,
		tl_r + handle * Vector2{-1, 0},
		tl_b + handle * Vector2{0, 1},
		tl_b
	));
	outerContour.addEdge(new LinearSegment(tl_b, bl_t));
	outerContour.addEdge(new CubicSegment(
		bl_t,
		bl_t + handle * Vector2{0, -1},
		bl_r + handle * Vector2{-1, 0},
		bl_r
	));
}

constexpr unsigned boarder_size = 128;
constexpr double boarder_range = 4;

export
svg_info create_boarder(double radius = 15., double width = 2., double k = .7f);

export
svg_info create_solid_boarder(double radius = 15., double k = .7f);
}
