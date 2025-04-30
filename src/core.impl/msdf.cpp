module;

#define MSDFGEN_USE_CPP11
#include <msdfgen/msdfgen.h>
#include <msdfgen/msdfgen-ext.h>
#include <nanosvg/nanosvg.h>

module mo_yanxi.graphic.msdf;

import std;


mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::load_shape(
	const svg_info& shape,
	unsigned w,
	unsigned h,
	double range, int boarder){
	bitmap bitmap = {w + boarder * 2, h + boarder * 2};

	const auto bound = shape.shape.getBounds();

	const double width = shape.size.x;
	const double height = shape.size.y;

	auto scale = bitmap.extent().sub(boarder * 2, boarder * 2).as<double>().copy().div(width, height);

	msdfgen::Projection projection(
		msdfgen::Vector2(
			scale.x,
			-scale.y)
		, msdfgen::Vector2{
			+ (static_cast<double>(boarder)) / scale.x,
			-bound.t - bound.b - (static_cast<double>(boarder)) / scale.y
		}
	);

	msdfgen::Bitmap<float, 3> fbitmap(bitmap.width(), bitmap.height());
	msdfgen::generateMSDF(fbitmap, shape.shape, projection, range);

	msdfgen::simulate8bit(fbitmap);
	msdf::write_to_bitmap(bitmap, fbitmap);
	return bitmap;
}

mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::load_glyph(
	msdfgen::FontHandle* face, msdfgen::unicode_t code,
	unsigned target_w, unsigned target_h, int boarder,
	double font_w, double font_h,
	double range){
	using namespace msdfgen;
	Shape shape;

	if(loadGlyph(shape, face, code, FONT_SCALING_EM_NORMALIZED)){
		bitmap bitmap = {target_w + boarder * 2, target_h + boarder * 2};

		shape.orientContours();
		shape.normalize();

		const auto bound = shape.getBounds();
		const math::vector2 scale{font_w, font_h};

		edgeColoringSimple(shape, 2.5);
		Bitmap<float, 3> msdf(bitmap.width(), bitmap.height());

		auto offx = -bound.l + static_cast<double>(boarder) / scale.x;
		auto offy = -bound.t - static_cast<double>(boarder) / scale.y;

		SDFTransformation t(
			Projection(
				Vector2{
					scale.x,
					-scale.y
				}, Vector2(
					offx,
					offy
				)), Range(range));
		generateMSDF(msdf, shape, t);
		simulate8bit(msdf);

		write_to_bitmap(bitmap, msdf);

		return bitmap;
	}

	//TODO ...
	return {target_w + boarder * 2, target_h + boarder * 2};
}

mo_yanxi::graphic::msdf::svg_info mo_yanxi::graphic::msdf::svg_to_shape(const char* path){
	using namespace msdfgen;


	NSVGimage* svgImage = nsvgParseFromFile(path, "px", 96.0f);
	if(!svgImage){
		return {};
	}


	Shape shape;
	for(NSVGshape* svgShape = svgImage->shapes; svgShape; svgShape = svgShape->next){
		for(NSVGpath* svgPath = svgShape->paths; svgPath; svgPath = svgPath->next){
			Contour& contour = shape.addContour();

			for(const auto& [p1, p2, p3, p4] :
				std::span<math::vec2>{reinterpret_cast<math::vec2*>(svgPath->pts), static_cast<std::size_t>(svgPath->npts)}
				| std::views::adjacent<4>
				| std::views::stride(3)){
				Point2 p1_(p1.x, svgImage->height - p1.y);
				Point2 p2_(p2.x, svgImage->height - p2.y);
				Point2 p3_(p3.x, svgImage->height - p3.y);
				Point2 p4_(p4.x, svgImage->height - p4.y);

				contour.addEdge(new CubicSegment(p1_, p2_, p3_, p4_));
				}
		}
	}

	math::vec2 sz{svgImage->width, svgImage->height};

	nsvgDelete(svgImage);
	shape.orientContours();
	edgeColoringByDistance(shape, 2.);
	shape.normalize();

	return {shape, sz};
}

mo_yanxi::graphic::msdf::msdf_generator::msdf_generator(svg_info&& shape, double range, int boarder): shape(std::move(shape)),
	range(range),
	boarder(boarder){
}

mo_yanxi::graphic::msdf::msdf_generator::msdf_generator(const char* path, double range, int boarder): shape(svg_to_shape(path)),
	range(range),
	boarder(boarder){
}

//
// mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::load_svg(const char* path, unsigned w, unsigned h, double range){
// 	return load_shape(svg_to_shape(path), w, h, range);
// }


mo_yanxi::graphic::msdf::svg_info mo_yanxi::graphic::msdf::create_boarder(double radius, double width, double k){
	using namespace msdfgen;

	// 创建形状对象
	Shape shape;
	shape.inverseYAxis = true; // 翻转Y轴坐标（视需求而定）


	add_contour(shape, boarder_size, radius, k);
	add_contour(shape, boarder_size, radius - width, k, width);
	// 验证形状有效性
	if(!shape.validate()) return {};


	shape.orientContours();
	edgeColoringByDistance(shape, 2.);
	shape.normalize();

	return {shape, {boarder_size, boarder_size}};
}

mo_yanxi::graphic::msdf::svg_info mo_yanxi::graphic::msdf::create_solid_boarder(double radius, double k){
	using namespace msdfgen;

	// 创建形状对象
	Shape shape;
	shape.inverseYAxis = true; // 翻转Y轴坐标（视需求而定）

	constexpr double strokeWidth = 2.0; // 轮廓线宽

	add_contour(shape, boarder_size, radius, k);
	// add_contour(shape, 64, radius - strokeWidth, k, strokeWidth);
	// 验证形状有效性
	if(!shape.validate()) return {};

	shape.orientContours();
	edgeColoringByDistance(shape, 2.);
	shape.normalize();
	// 清理资源
	return {shape, {boarder_size, boarder_size}};
}
