// ReSharper disable CppDFANotInitializedField
module;


#include <vulkan/vulkan.h>
#include "../src/ext/assume.hpp"
#include "../src/ext/adapted_attributes.hpp"

export module mo_yanxi.graphic.image_nine_region;

export import mo_yanxi.graphic.image_region;
import mo_yanxi.graphic.grid_generator;
import mo_yanxi.math.quad;
import align;

import std;
import mo_yanxi.meta_programming;

namespace mo_yanxi::graphic{
	constexpr align::scale DefaultScale = align::scale::stretch;

	export
	struct image_caped_region{
		static constexpr auto size = 3;
		using region_type = combined_image_region<size_awared_uv<uniformed_rect_uv>>;

	private:
		math::vec2 total_region{};
		float src_len{};
		float end_len{};

		sized_image image_view{};
		std::array<uniformed_rect_uv, size> regions{};

	public:
		float margin{};

		[[nodiscard]] image_caped_region() = default;

		constexpr image_caped_region(
			const region_type& imageRegion,
			math::urect region,
			const float src_len,
			const float end_len,
			const float external_margin = 0.f
		) : src_len(src_len), end_len(end_len), image_view(imageRegion), margin(external_margin){
			auto cap_inner_len = cap_width();

			auto src_len_ = src_len;
			auto end_len_ = end_len;

			float len = region.width();

			if(cap_inner_len > len){
				src_len_ = len * src_len_ / cap_inner_len;
				end_len_ = len * end_len_ / cap_inner_len;
			}

			total_region = {len, static_cast<float>(region.height())};

			const math::rect_ortho<double>
				rect_src{tags::unchecked, tags::from_extent, region.src.as<double>() + math::vector2<double>{}, {src_len_, total_region.y}};

			const math::rect_ortho<double>
				rect_dst{tags::unchecked, tags::from_extent, region.src.as<double>() + math::vector2<double>{len - end_len_}, {end_len_, total_region.y}};

			const math::rect_ortho<double> rect_mid{rect_src.vert_10(), rect_dst.vert_01()};

			regions[0].fetch_into(image_view.size.as<double>(), rect_src);
			regions[1].fetch_into(image_view.size.as<double>(), rect_mid);
			regions[2].fetch_into(image_view.size.as<double>(), rect_dst);
		}

		[[nodiscard]] VkImageView get_image_view() const noexcept{
			return image_view.view;
		}

		[[nodiscard]] FORCE_INLINE constexpr float mid_width() const noexcept{
			return end_len - src_len;
		}
		[[nodiscard]] FORCE_INLINE constexpr float cap_width() const noexcept{
			return src_len + end_len;
		}

		[[nodiscard]] constexpr std::span<const uniformed_rect_uv, size> get_uvs() const noexcept{
			return regions;
		}

		constexpr uniformed_rect_uv operator[](std::size_t idx) const noexcept{
			return regions[idx];
		}

		[[nodiscard]] math::vec2 get_size() const noexcept{
			return total_region;
		}

		[[nodiscard]] std::array<math::fquad, size> get_regions(math::vec2 src, math::vec2 dst, float scale = 1.) const noexcept{
			auto approach = dst - src;
			auto len = approach.length();

			auto cap_inner_len = cap_width() / 2.f;

			auto src_len_ = src_len / 2.f;
			auto end_len_ = end_len / 2.f;

			if(cap_inner_len > len){
				src_len_ = len * src_len_ / cap_inner_len;
				end_len_ = len * end_len_ / cap_inner_len;
			}

			math::frect rect_src{tags::unchecked, tags::from_extent, {}, math::vec2{src_len_ * 2, total_region.y} * scale};
			math::frect rect_dst{tags::unchecked, tags::from_extent, {}, math::vec2{end_len_ * 2, total_region.y} * scale};

			const auto cos = approach.x / len;
			const auto sin = approach.y / len;
			rect_src.set_center(src).expand(margin / 2, margin);
			rect_dst.set_center(dst).expand(margin / 2, margin);

			auto quad_src = math::rect_ortho_to_quad(rect_src, rect_src.get_center(),  cos,  sin);
			auto quad_dst = math::rect_ortho_to_quad(rect_dst, rect_dst.get_center(),  cos,  sin);
			quad_src.move(math::vec2{margin / 2}.rotate(-cos, -sin));
			quad_dst.move(math::vec2{margin / 2}.rotate( cos,  sin));

			math::fquad quad_mid{quad_src.v1, quad_dst.v0, quad_dst.v3, quad_src.v2};

			return {quad_src, quad_mid, quad_dst};
		}
	};


	// using T = float;
	template <typename T = float>
		requires (std::is_arithmetic_v<T>)
	using Generator = grid_generator<4, T>;

	using NinePatchProp = grid_property<4>;
	constexpr auto NinePatchSize = NinePatchProp::size;

	export
	template <typename T = float>
		requires (std::is_arithmetic_v<T>)
	struct nine_patch_raw{
		using rect = math::rect_ortho<T>;

		std::array<rect, NinePatchSize> values{};

		[[nodiscard]] constexpr nine_patch_raw() noexcept = default;

		[[nodiscard]] constexpr nine_patch_raw(const rect internal, const rect external) noexcept
			: values{
				graphic::create_grid<4, T>({
						external.vert_00(),
						internal.vert_00(),
						internal.vert_11(),
						external.vert_11()
					})
			}{}

		[[nodiscard]] constexpr nine_patch_raw(align::padding<T> edge, const rect external) noexcept
			{
			constexpr T err = std::numeric_limits<T>::epsilon() * 32;

			bool need_scale{false};
			float ratio_l;
			float ratio_b;
			if(edge.width() >= external.width()){
				need_scale = true;
				ratio_l = edge.left / edge.width();
			}else{
				ratio_l = .5f;
			}

			if(edge.height() >= external.height()){
				need_scale = true;
				ratio_b = edge.bottom / edge.height();
			}else{
				ratio_b = .5f;
			}

			if(need_scale){
				auto target_bot_lft = external.size().scl(ratio_l - err, ratio_b - err).to_abs();
				auto target_top_rit = external.size().scl(1 - ratio_l - err, 1 - ratio_b - err).to_abs();

				auto true_botlft = align::embedTo(align::scale::fit, edge.bot_lft(), target_bot_lft);
				auto true_toprit = align::embedTo(align::scale::fit, edge.top_rit(), target_top_rit);

				values = graphic::create_grid<4, T>({
							external.vert_00(),
							external.vert_00() + true_botlft,
							external.vert_11() - true_toprit,
							external.vert_11()
						});
			}else{
				values = graphic::create_grid<4, T>({
						external.vert_00(),
						external.vert_00() + edge.bot_lft(),
						external.vert_11() - edge.top_rit(),
						external.vert_11()
					});
			}

		}


		[[nodiscard]] constexpr nine_patch_raw(
			const align::spacing edge,
			const rect rect,
			const math::vector2<T> centerSize,
			const align::scale centerScale) noexcept
			: nine_patch_raw{edge, rect}{

			nine_patch_raw::set_center_scale(centerSize, centerScale);
		}


		[[nodiscard]] constexpr nine_patch_raw(
			const rect internal, const rect external,
			const math::vector2<T> centerSize,
			const align::scale centerScale) noexcept
			: nine_patch_raw{internal, external}{

			nine_patch_raw::set_center_scale(centerSize, centerScale);
		}

		constexpr void set_center_scale(const math::vector2<T> centerSize, const align::scale centerScale) noexcept{
			if(centerScale != DefaultScale){
				const auto sz = align::embedTo(centerScale, centerSize, center().size());
				const auto offset = align::get_offset_of<to_signed_t<T>>(align::pos::center, sz.as_signed(), center().as_signed());
				center() = {tags::from_extent, static_cast<math::vector2<T>>(offset), sz};
			}
		}

		constexpr rect operator [](const unsigned index) const noexcept{
			return values[index];
		}

		[[nodiscard]] constexpr math::rect_ortho<T>& center() noexcept{
			return values[grid_property<4>::center_index];
		}

		[[nodiscard]] constexpr const math::rect_ortho<T>& center() const noexcept{
			return values[grid_property<4>::center_index];
		}

	};

	export
	struct nine_patch_brief{
		align::spacing edge{};
		math::vec2 inner_size{};
		align::scale center_scale{DefaultScale};

		[[nodiscard]] constexpr nine_patch_raw<float> get_regions(const nine_patch_raw<float>::rect bound) const noexcept{
			return nine_patch_raw{edge, bound, inner_size, center_scale};
		}

		[[nodiscard]] constexpr math::vec2 get_recommended_size() const noexcept{
			return inner_size + edge.get_size();
		}

		[[nodiscard]] nine_patch_brief() = default;


		[[nodiscard]] nine_patch_brief(
			math::urect external,
			math::urect internal,
			const math::usize2 centerSize = {},
			const align::scale centerScale = DefaultScale
		){
			this->center_scale = centerScale;
			this->inner_size = centerSize.as<float>();
			this->edge = align::padBetween(internal.as<float>(), external.as<float>());
		}


		[[nodiscard]] math::vec2 get_size() const noexcept{
			return inner_size + edge.get_size();
		}

	};

	export
	struct image_nine_region : nine_patch_brief{
		static constexpr auto size = NinePatchSize;
		using region_type = combined_image_region<size_awared_uv<uniformed_rect_uv>>;

		sized_image image_view{};
		std::array<uniformed_rect_uv, NinePatchSize> regions{};
		float margin{};

		[[nodiscard]] image_nine_region() = default;

		image_nine_region(
			const region_type& imageRegion,
			math::urect internal_in_relative,
			const float external_margin = 0.f,
			const math::usize2 centerSize = {},
			const align::scale centerScale = DefaultScale,
			const float edgeShrinkScale = 0.25f
		) : image_view(imageRegion), margin(external_margin){

			const auto external = imageRegion.uv.get_region();
			internal_in_relative.src += external.src;
			CHECKED_ASSUME(external.contains_loose(internal_in_relative));
			this->nine_patch_brief::operator=(nine_patch_brief{external, internal_in_relative, centerSize, centerScale});



			using gen = Generator<float>;
			const auto ninePatch = nine_patch_raw{internal_in_relative, external, centerSize, centerScale};
			for (auto && [i, region] : regions | std::views::enumerate){
				region.fetch_into(image_view.size, ninePatch[i]);
			}

			for (const auto hori_edge_index : gen::property::edge_indices | std::views::take(gen::property::edge_indices.size() / 2)){
				regions[hori_edge_index].shrink(image_view.size, {edgeShrinkScale * inner_size.x, 0});
			}

			for (const auto vert_edge_index : gen::property::edge_indices | std::views::drop(gen::property::edge_indices.size() / 2)){
				regions[vert_edge_index].shrink(image_view.size, {0, edgeShrinkScale * inner_size.y});
			}
			//
			// for (auto && uvData : regions){
			// 	uvData.flipY();
			// }

		}

		image_nine_region(
			const region_type& imageRegion,
			const align::padding<std::uint32_t> margin,
			const float external_margin = 0.f,
			const math::usize2 centerSize = {},
			const align::scale centerScale = DefaultScale,
			const float edgeShrinkScale = 0.25f
		) : image_nine_region{imageRegion, math::urect{
			tags::from_extent, margin.bot_lft(), imageRegion.uv.get_region().size().sub(margin.get_size())
		}, external_margin, centerSize, centerScale, edgeShrinkScale}{
			assert(margin.get_size().within(imageRegion.uv.get_region().size()));
		}

		// ImageViewNineRegion(
		// 	const region_type& external,
		// 	const Align::padding<std::uint32_t> margin,
		// 	const region_type& internal,
		// 	const Align::Scale centerScale = DefaultScale,
		// 	const float edgeShrinkScale = 0.25f
		// ) : ImageViewNineRegion{
		// 		external,
		// 		margin,
		// 		internal.getRegionSize(),
		// 		centerScale,
		// 		edgeShrinkScale
		// 	}{
		//
		// 	assert(static_cast<region_type&>(external) == static_cast<region_type&>(internal));
		//
		// 	regions[NinePatchProp::center_index] = UVData(internal);
		// }
		//
		//
		// ImageViewNineRegion(
		// 	const ImageViewRegion& external,
		// 	const Align::padding<std::uint32_t> margin,
		// 	const ImageViewRegion& internal,
		// 	const Geom::USize2 centerSize,
		// 	const Align::Scale centerScale = DefaultScale,
		// 	const float edgeShrinkScale = 0.25f
		// ) : ImageViewNineRegion{
		// 		external,
		// 		margin,
		// 		centerSize,
		// 		centerScale,
		// 		edgeShrinkScale,
		// 	}{
		//
		// 	assert(static_cast<const SizedImageView&>(external) == static_cast<const SizedImageView&>(internal));
		//
		// 	regions[NinePatchProp::center_index] = UVData(internal);
		// }


	};
}
