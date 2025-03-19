module;

export module mo_yanxi.graphic.msdf;

export import mo_yanxi.graphic.bitmap;
import mo_yanxi.math;
import std;


import <msdfgen/msdfgen.h>;
import <msdfgen/msdfgen-ext.h>;

namespace mo_yanxi::graphic::msdf{
	export constexpr inline std::uint32_t sdf_image_boarder = 8;
	export constexpr inline double sdf_image_range = 1.5f;

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
				bit.r = bit.g = bit.b = math::round<std::uint8_t>(region(x, y)[0] * static_cast<float>(std::numeric_limits<std::uint8_t>::max()));
				bit.a = std::numeric_limits<std::uint8_t>::max();
			}
		}
	}

	export
	[[nodiscard]] bitmap load_shape(msdfgen::Shape&& shape, unsigned w, unsigned h, double range = sdf_image_range);


	export
	[[nodiscard]] bitmap load_glyph(
		msdfgen::FontHandle* face,
		msdfgen::unicode_t code,
		unsigned target_w,
		unsigned target_h,
		unsigned font_w,
		unsigned font_h,
		double range = sdf_image_range
		);

	export
	[[nodiscard]] bitmap load_svg(const char* path, unsigned w, unsigned h, double range = 2.);


	void add_contour(msdfgen::Shape& shape, double size, double radius, double k, double margin = 0.f, msdfgen::Vector2 offset = {}){
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

		using Segment = QuadraticSegment;

		Vector2 handle{k * radius, k * radius};

		// 外轮廓路径（顺时针）
		outerContour.addEdge(new LinearSegment(bl_r, br_l));
		outerContour.addEdge(new CubicSegment(
			br_l,
			br_l + handle * Vector2{ 1,  0},
			br_t + handle * Vector2{ 0, -1},
			br_t
		));
		outerContour.addEdge(new LinearSegment(br_t, tr_b));
		outerContour.addEdge(new CubicSegment(
			tr_b,
			tr_b + handle * Vector2{ 0,  1},
			tr_l + handle * Vector2{ 1,  0},
			tr_l
		));
		outerContour.addEdge(new LinearSegment(tr_l, tl_r));
		outerContour.addEdge(new CubicSegment(
			tl_r,
			tl_r + handle * Vector2{-1,  0},
			tl_b + handle * Vector2{ 0,  1},
			tl_b
		));
		outerContour.addEdge(new LinearSegment(tl_b, bl_t));
		outerContour.addEdge(new CubicSegment(
			bl_t,
			bl_t + handle * Vector2{ 0, -1},
			bl_r + handle * Vector2{-1,  0},
			bl_r
		));
	}

	constexpr unsigned boarder_size = 128;
	constexpr double boarder_range = 4;

	export
	bitmap create_boarder(double radius = 15., double k = .7f, double width = 2.);

	export
	bitmap create_solid_boarder(double radius = 15., double k = .7f);
}
