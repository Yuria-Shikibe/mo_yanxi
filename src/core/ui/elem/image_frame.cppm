//
// Created by Matrix on 2024/9/24.
//

export module mo_yanxi.ui.elem.image_frame;

export import mo_yanxi.ui.elem;
export import mo_yanxi.ui.image_drawable;
export import mo_yanxi.graphic.color;
export import align;
// export import Core.UI.RegionDrawable;

import std;

namespace mo_yanxi::ui{
	//TODO ImageDisplay with static type

	export
	struct image_display_style{
		align::scale scaling{align::scale::fit};
		align::pos align{align::pos::center};
		graphic::color mix_color{};
		// float mixScale{};
	};

	export
	struct image_frame : public elem{
		image_display_style style{};

		[[nodiscard]] image_frame(scene* scene, group* group)
			: elem(scene, group, "image_frame"){
		}

		template <std::derived_from<drawable> Ty, typename ...T>
			requires (std::constructible_from<Ty, T...>)
		void set_drawable(T&& ...args){
			image_drawable = std::make_unique<Ty>(std::forward<T>(args) ...);
		}

	protected:
		std::unique_ptr<image_drawable> image_drawable;

		[[nodiscard]] static math::vec2 get_expected_size(const drawable& drawable, const image_display_style& style, math::vec2 bound) noexcept{
			if(const auto sz = drawable.get_default_size()){
				return align::embedTo(style.scaling, *sz, bound);
			}

			return bound;
		}

		void draw_content(const rect clipSpace, const rect redirect) const override{
			if(!image_drawable)return;
			auto sz = get_expected_size(*image_drawable, style, content_size());
			auto off = align::get_offset_of(style.align, sz, property.content_bound_absolute());
			image_drawable->draw(*this, rect{tags::from_extent, off, sz}, graphicProp().style_color_scl, style.mix_color);
		}
	};

	/*struct ImageDisplay_SingleStyle : image_frame{
		image_display_style style{};

		[[nodiscard]] explicit(false) ImageDisplay_SingleStyle(
			const std::string_view tyName,
			const image_display_style& style = {})
			: image_frame{tyName},
			  style{style}{}

		[[nodiscard]] explicit(false) ImageDisplay_SingleStyle(const image_display_style& style = {})
			: ImageDisplay_SingleStyle{"", style}{}
	};

	export
	struct ImageDisplay : public ImageDisplay_SingleStyle{
	private:
		std::unique_ptr<RegionDrawable> drawable{};
		std::move_only_function<decltype(drawable)(ImageDisplay&)> drawableProv{};

	public:
		[[nodiscard]] ImageDisplay() : ImageDisplay_SingleStyle{"ImageDisplay"}{}

		template <typename Drawable>
			requires std::derived_from<std::decay_t<Drawable>, RegionDrawable>
		[[nodiscard]] explicit ImageDisplay(
			Drawable&& drawable,
			const image_display_style& style = {})
			:
			ImageDisplay_SingleStyle{"ImageDisplay", style},
			drawable{std::make_unique<std::decay_t<Drawable>>(std::forward<Drawable>(drawable))}{}

		[[nodiscard]] bool isDynamic() const noexcept{
			return drawableProv != nullptr;
		}

		template <typename Fn>
		void setProvider(Fn&& fn){
			Util::setFunction(drawableProv, std::forward<Fn>(fn));
		}

		void setDrawable(std::unique_ptr<RegionDrawable>&& drawable){
			this->drawable = std::move(drawable);
		}

		template <typename Drawable>
			requires std::derived_from<std::decay_t<Drawable>, RegionDrawable>
		void setDrawable(Drawable&& drawable){
			this->drawable = std::make_unique<std::decay_t<Drawable>>(std::forward<Drawable>(drawable));
		}

		void update(const float delta_in_tick) override{
			elem::update(delta_in_tick);

			if(isDynamic()){
				drawable = drawableProv(*this);
			}

			if(drawable)this->updateImage(*drawable, style);
		}

		void drawMain(Rect clipSpace) const override;
	};

	export
	template <std::derived_from<RegionDrawable> Drawable>
	struct FixedImageDisplay : public ImageDisplay_SingleStyle{
	private:
		Drawable drawable;
		std::move_only_function<decltype(drawable)(FixedImageDisplay&)> drawableProv{};

	public:
		[[nodiscard]] FixedImageDisplay() requires(std::is_default_constructible_v<Drawable>)
			: ImageDisplay_SingleStyle{"ImageDisplay"}, drawable{}{}

		template <std::convertible_to<Drawable> Region>
		[[nodiscard]] explicit FixedImageDisplay(
			Region&& drawable,
			const image_display_style& style = {})
			:
			ImageDisplay_SingleStyle{"ImageDisplay", style},
			drawable{std::forward<Region>(drawable)}{
		}

		[[nodiscard]] bool isDynamic() const noexcept{
			return drawableProv != nullptr;
		}

		template <typename Fn>
		void setProvider(Fn&& fn){
			Util::setFunction(drawableProv, std::forward<Fn>(fn));
		}

		template <std::convertible_to<Drawable> Region>
		void setDrawable(Region&& drawable){
			this->drawable = std::forward<Region>(drawable);
		}

		void update(const float delta_in_tick) override{
			elem::update(delta_in_tick);

			if(isDynamic()){
				drawable = drawableProv(*this);
			}

			this->updateImage(drawable, style);

		}

		void drawMain(const Rect clipSpace) const override{
			elem::drawMain(clipSpace);

			this->drawDrawable(drawable, style);
		}
	};


	export
	template <typename Drawable>
		requires (std::derived_from<std::decay_t<Drawable>, RegionDrawable>)
	FixedImageDisplay(Drawable&&) -> FixedImageDisplay<std::decay_t<Drawable>>;

	export
	template <typename Drawable>
		requires (std::derived_from<std::decay_t<Drawable>, RegionDrawable>)
	FixedImageDisplay(Drawable&&, image_display_style) -> FixedImageDisplay<std::decay_t<Drawable>>;
	*/

}
