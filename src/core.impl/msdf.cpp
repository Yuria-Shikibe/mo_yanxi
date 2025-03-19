module;

#define MSDFGEN_USE_CPP11
#include <msdfgen/msdfgen.h>
#include <msdfgen/msdfgen-ext.h>
#include <nanosvg/nanosvg.h>

module mo_yanxi.graphic.msdf;


mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::load_shape(msdfgen::Shape&& shape, unsigned w, unsigned h,
                                                              double range){
	bitmap bitmap = {w + sdf_image_boarder, h + sdf_image_boarder};

	msdfgen::edgeColoringByDistance(shape, 2.);

	const auto bound = shape.getBounds();

	const double width = bound.r - bound.l;
	const double height = bound.t - bound.b;

	auto scale = bitmap.extent().sub(sdf_image_boarder, sdf_image_boarder).as<double>().div(width, height);
	// shape.normalize();
	msdfgen::Projection projection(
		msdfgen::Vector2(
			scale.x,
			-scale.y)
		, msdfgen::Vector2(
			-bound.l + sdf_image_boarder / 2. / scale.x,
			-bound.b - height - sdf_image_boarder / 2. / scale.y
		)
	);

	// 6. 生成MSDF位图
	msdfgen::Bitmap<float, 3> fbitmap(bitmap.width(), bitmap.height());
	// msdfgen::generateSDF(fbitmap, shape, projection, range);
	msdfgen::generateMSDF(fbitmap, shape, projection, range);

	msdfgen::simulate8bit(fbitmap);
	msdf::write_to_bitmap(bitmap, fbitmap);
	return bitmap;
}

mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::load_glyph(msdfgen::FontHandle* face, msdfgen::unicode_t code,
                                                              unsigned target_w, unsigned target_h, unsigned font_w,
                                                              unsigned font_h, double range){
	using namespace msdfgen;
	Shape shape;

	if(loadGlyph(shape, face, code, FONT_SCALING_EM_NORMALIZED)){
		bitmap bitmap = {target_w + sdf_image_boarder, target_h + sdf_image_boarder};

		shape.orientContours();
		shape.normalize();

		const auto bound = shape.getBounds();
		const double height = bound.t - bound.b;
		const auto scale = math::vector2{font_w, font_h}.as<double>();

		edgeColoringSimple(shape, 2.5);
		Bitmap<float, 3> msdf(bitmap.width(), bitmap.height());

		auto offx = -bound.l + sdf_image_boarder / 2. / scale.x;
		auto offy = -bound.b - height - sdf_image_boarder / 2. / scale.y;

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

	return {};
}

mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::load_svg(const char* path, unsigned w, unsigned h, double range){
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
			    std::span{reinterpret_cast<math::vec2*>(svgPath->pts), static_cast<std::size_t>(svgPath->npts)}
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

	nsvgDelete(svgImage);

	shape.normalize();
	shape.orientContours();
	edgeColoringSimple(shape, 1.5);

	return load_shape(std::move(shape), w, h, range);
}

mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::create_boarder(double radius, double k, double width){
	using namespace msdfgen;

	// 创建形状对象
	Shape shape;
	shape.inverseYAxis = true; // 翻转Y轴坐标（视需求而定）


	add_contour(shape, boarder_size, radius, k);
	add_contour(shape, boarder_size, radius - width, k, width);
	// 验证形状有效性
	if(!shape.validate()) return {};

	shape.orientContours();
	auto mp = load_shape(std::move(shape), boarder_size, boarder_size, 8);

	// 清理资源
	return mp;
}

mo_yanxi::graphic::bitmap mo_yanxi::graphic::msdf::create_solid_boarder(double radius, double k){
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
	auto mp = load_shape(std::move(shape), boarder_size, boarder_size, 8);

	// 清理资源
	return mp;
}
