// ReSharper disable CppDFANotInitializedField
module;


#include "../src/ext/assume.hpp"

export module mo_yanxi.graphic.image_nine_region;

export import mo_yanxi.graphic.image_region;
import mo_yanxi.graphic.grid_generator;
import align;

import std;
import mo_yanxi.meta_programming;

namespace mo_yanxi::graphic{
	constexpr align::scale DefaultScale = align::scale::stretch;


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
			constexpr T err = std::numeric_limits<T>::epsilon() * 4;
			if(edge.width() >= external.width()){
				float ratio = edge.left / edge.width();
				edge.left = (ratio - err) * external.width();
				edge.right = (1 - ratio - err) * external.width();
			}

			if(edge.height() >= external.height()){
				float ratio = edge.bottom / edge.height();
				edge.bottom = (ratio - err) * external.height();
				edge.top = (1 - ratio - err) * external.height();
			}

			values = graphic::create_grid<4, T>({
						external.vert_00(),
						external.vert_00() + edge.bot_lft(),
						external.vert_11() - edge.top_rit(),
						external.vert_11()
					});
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

		[[nodiscard]] constexpr nine_patch_raw<float> get_patches(const nine_patch_raw<float>::rect bound) const noexcept{
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


	};

	export
	struct image_multi_region : nine_patch_brief{
		static constexpr auto size = NinePatchSize;
		using region_type = combined_image_region<size_awared_uv<uniformed_rect_uv>>;

		sized_image image_view{};
		std::array<uniformed_rect_uv, NinePatchSize> regions{};
		float margin{};

		[[nodiscard]] image_multi_region() = default;

		image_multi_region(
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

		image_multi_region(
			const region_type& imageRegion,
			const align::padding<std::uint32_t> margin,
			const float external_margin = 0.f,
			const math::usize2 centerSize = {},
			const align::scale centerScale = DefaultScale,
			const float edgeShrinkScale = 0.25f
		) : image_multi_region{imageRegion, math::urect{
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
