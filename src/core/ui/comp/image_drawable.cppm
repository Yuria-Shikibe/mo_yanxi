//
// Created by Matrix on 2024/4/14.
//

export module mo_yanxi.ui.image_drawable;

export import mo_yanxi.ui.basic;
export import mo_yanxi.ui.style;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.vector2;
export import mo_yanxi.graphic.color;
export import mo_yanxi.graphic.image_manage;
export import mo_yanxi.graphic.image_region;
export import mo_yanxi.graphic.image_multi_region;
export import mo_yanxi.vk.vertex_info;
export import align;

import std;

namespace mo_yanxi::ui{
	//TODO using variant?
	export
	struct drawable{
		[[nodiscard]] drawable() = default;

		virtual ~drawable() = default;

		virtual explicit operator bool() const noexcept{
			return false;
		}

		virtual void draw(const elem& elem, math::frect region, graphic::color color_scl) const = 0;

		[[nodiscard]] virtual std::optional<math::vec2> get_default_size() const{
			return std::nullopt;
		}
	};


	export
	struct image_display_style{
		align::scale scaling{align::scale::fit};
		align::pos align{align::pos::center};
		style::palette palette{style::general_palette};
	};

	export
	struct styled_drawable{
		image_display_style style{};
		std::unique_ptr<drawable> drawable{};
	};

	struct moded_drawable : public drawable{
		vk::vertices::mode_flag_bits draw_flags{};

		[[nodiscard]] moded_drawable() = default;

		[[nodiscard]] explicit moded_drawable(vk::vertices::mode_flag_bits draw_flags)
			: draw_flags(draw_flags){
		}
	};

	export
	struct image_drawable final : public moded_drawable{
		graphic::cached_image_region image{};

		[[nodiscard]] image_drawable() = default;

		[[nodiscard]] image_drawable(vk::vertices::mode_flag_bits flags, graphic::allocated_image_region& image_region)
			: moded_drawable(flags), image(image_region){
		}

		explicit operator bool() const noexcept override{
			return image->view != nullptr;
		}

		void draw(const elem& elem, math::frect region, graphic::color color_scl) const override;

		[[nodiscard]] std::optional<math::vec2> get_default_size() const override {
			return image->get_region().size().as<float>();
		}
	};
	export
	struct icon_drawable final : public moded_drawable{
		using icon_type = graphic::combined_image_region<graphic::size_awared_uv<graphic::uniformed_rect_uv>>;
		icon_type image{};

		[[nodiscard]] icon_drawable() = default;

		[[nodiscard]] explicit(false) icon_drawable(const icon_type& image_region)
			: moded_drawable(vk::vertices::mode_flag_bits::sdf), image(image_region){
		}

		explicit operator bool() const noexcept override{
			return image.view != nullptr;
		}

		void draw(const elem& elem, math::frect region, graphic::color color_scl) const override;

		[[nodiscard]] std::optional<math::vec2> get_default_size() const override {
			return image.uv.size.as<float>();
		}
	};


	export
	struct drawable_ref final : public drawable{
		const drawable* ref{};

		[[nodiscard]] drawable_ref() = default;

		[[nodiscard]] explicit(false) drawable_ref(const drawable* ref)
			: ref(ref){
		}

		explicit operator bool() const noexcept override{
			return ref && *ref;
		}

		void draw(const elem& elem, math::frect region, graphic::color color_scl) const override{
			if(ref)ref->draw(elem, region, color_scl);
		}

		[[nodiscard]] std::optional<math::vec2> get_default_size() const override {
			if(ref)return ref->get_default_size();
			return std::nullopt;
		}
	};


	// struct RegionDrawable {
	// 	virtual ~RegionDrawable() = default;
	//
	// 	virtual void draw(Graphic::RendererUI& renderer, Geom::OrthoRectFloat rect, Graphic::Color color) const{
	//
	// 	}
	//
	// 	virtual bool requestSize(Geom::Vec2 expectedSize){
	// 		return false;
	// 	}
	//
	// 	[[nodiscard]] virtual Geom::Vec2 getRegionSize() const{
	// 		return {};
	// 	}
	//
	// 	[[nodiscard]] virtual Geom::Vec2 getRegionDefaultSize() const{
	// 		return getRegionSize();
	// 	}
	// };
	//
	// struct ImageRegionDrawable : RegionDrawable{
	// 	Graphic::ImageViewRegion region{};
	//
	// 	[[nodiscard]] constexpr ImageRegionDrawable() = default;
	//
	// 	[[nodiscard]] explicit constexpr ImageRegionDrawable(const Graphic::ImageViewRegion& region)
	// 		: region{region}{}
	//
	// 	void draw(Graphic::RendererUI& renderer, Geom::OrthoRectFloat rect, Graphic::Color color) const override;
	//
	// 	[[nodiscard]] Geom::Vec2 getRegionSize() const override{
	// 		return region.getRegionSize().as<float>();
	// 	}
	// };
	//
	// using Icon = ImageRegionDrawable;
	//
	// struct TextureNineRegionDrawable_Ref : RegionDrawable{
	// 	Graphic::ImageViewNineRegion* region{nullptr};
	//
	// 	[[nodiscard]] TextureNineRegionDrawable_Ref() = default;
	//
	// 	[[nodiscard]] explicit TextureNineRegionDrawable_Ref(Graphic::ImageViewNineRegion* const rect)
	// 		: region(rect) {
	// 	}
	//
	// 	void draw(Graphic::RendererUI& renderer, Geom::OrthoRectFloat rect, Graphic::Color color) const override;
	//
	// 	[[nodiscard]] Geom::Vec2 getRegionSize() const override{
	// 		return region->getRecommendedSize();
	// 	}
	// };
	//

	export
	struct image_caped_region_drawable final : moded_drawable{
		graphic::image_caped_region region{};
		float scale{1.f};

		[[nodiscard]] image_caped_region_drawable() = default;

		[[nodiscard]] image_caped_region_drawable(
			vk::vertices::mode_flag_bits draw_flags,
			const graphic::image_caped_region& region,
			float scale = 1.)
			: moded_drawable(draw_flags),
			  region(region), scale(scale){
		}

		void draw(const elem& elem, math::frect region, graphic::color color_scl) const override;

		[[nodiscard]] std::optional<math::vec2> get_default_size() const override{
			return region.get_size() * scale;
		}
	};

	export
	struct image_nine_region_drawable final : moded_drawable{
		graphic::image_nine_region region{};

		[[nodiscard]] image_nine_region_drawable() = default;

		[[nodiscard]] image_nine_region_drawable(
			vk::vertices::mode_flag_bits draw_flags,
			const graphic::image_nine_region& region)
			: moded_drawable(draw_flags),
			  region(region){
		}

		void draw(const elem& elem, math::frect region, graphic::color color_scl) const override;

		[[nodiscard]] std::optional<math::vec2> get_default_size() const override{
			return region.get_size();
		}
	};


	// struct UniqueRegionDrawable : RegionDrawable{
	// 	std::unique_ptr<GL::Texture2D> texture{};
	// 	GL::TextureRegion wrapper{};
	//
	// 	template <typename ...T>
	// 	explicit UniqueRegionDrawable(T&& ...args) : texture{std::make_unique<GL::Texture2D>(std::forward<T>(args)...)}, wrapper{texture.get()}{}
	//
	// 	explicit UniqueRegionDrawable(std::unique_ptr<GL::Texture2D>&& texture) : texture{std::move(texture)}, wrapper{texture.get()}{}
	//
	// 	explicit UniqueRegionDrawable(GL::Texture2D&& texture) : texture{std::make_unique<GL::Texture2D>(std::move(texture))}, wrapper{this->texture.get()}{}
	//
	// 	void draw(const Geom::OrthoRectFloat rect) const override;
	//
	// 	Geom::Vec2 getDefSize() const override{
	// 		return wrapper.getSize();
	// 	}
	// };


}
