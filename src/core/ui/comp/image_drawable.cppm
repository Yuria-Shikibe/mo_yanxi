//
// Created by Matrix on 2024/4/14.
//

export module mo_yanxi.ui.image_drawable;

export import mo_yanxi.ui.pre_decl;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.vector2;
export import mo_yanxi.graphic.color;
export import mo_yanxi.graphic.image_atlas;
export import mo_yanxi.graphic.image_region;
export import mo_yanxi.graphic.image_nine_region;
export import mo_yanxi.vk.vertex_info;

import std;

namespace mo_yanxi::ui{
	//TODO using variant?
	export
	struct drawable{
		virtual ~drawable() = default;

		virtual void draw(const elem& elem, math::frect region, graphic::color color_scl, graphic::color color_ovr) = 0;

		[[nodiscard]] virtual std::optional<math::vec2> get_default_size() const{
			return std::nullopt;
		}
	};

	export
	struct image_drawable : public drawable{
		vk::vertices::mode_flag_bits draw_flags{};
		graphic::cached_image_region image{};

		[[nodiscard]] image_drawable() = default;

		[[nodiscard]] image_drawable(vk::vertices::mode_flag_bits flags, graphic::allocated_image_region& image_region)
			: draw_flags(flags), image(image_region){
		}

		void draw(const elem& elem, math::frect region, graphic::color color_scl, graphic::color color_ovr) override;

		[[nodiscard]] std::optional<math::vec2> get_default_size() const override {
			return image->get_region().size().as<float>();
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
	// struct TextureNineRegionDrawable_Owner : RegionDrawable{
	// 	Graphic::ImageViewNineRegion region{};
	//
	// 	[[nodiscard]] TextureNineRegionDrawable_Owner() = default;
	//
	// 	[[nodiscard]] explicit(false) TextureNineRegionDrawable_Owner(const Graphic::ImageViewNineRegion& rect)
	// 		: region(rect) {
	// 	}
	//
	// 	void draw(Graphic::RendererUI& renderer, Geom::OrthoRectFloat rect, Graphic::Color color) const override;
	//
	// 	[[nodiscard]] Geom::Vec2 getRegionSize() const override{
	// 		return region.getRecommendedSize();
	// 	}
	// };


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
