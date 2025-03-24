export module mo_yanxi.ui.elem.image_frame;

export import mo_yanxi.ui.elem;
export import mo_yanxi.ui.image_drawable;
export import mo_yanxi.graphic.color;
export import align;

import std;

namespace mo_yanxi::ui{

	export
	struct image_frame : public elem{
		image_display_style default_style{};

	protected:
		std::size_t current_frame_index{};
		std::vector<styled_drawable> drawables_{};
		//
		// [[nodiscard]] image_frame(scene* scene, group* group, const std::string_view tyName)
		// 	: elem(scene, group, tyName){
		// }

		// using elem::elem;

	public:
		[[nodiscard]] image_frame(scene* scene, group* group)
			: elem(scene, group, "image_frame"){
		}

		void add_collapser_image_swapper(){
			spreadable_event_handler_on<events::collapser_state_changed>([](events::collapser_state_changed e, image_frame& self){
				self.current_frame_index = e.expanded;
				return false;
			});
		}

		template <std::derived_from<drawable> Ty, typename ...T>
			requires (std::constructible_from<Ty, T...>)
		void set_drawable(const std::size_t idx, image_display_style style, T&& ...args){
			drawables_.resize(math::max<std::size_t>(drawables_.size(), idx + 1));
			drawables_[idx] = styled_drawable{style, std::make_unique<Ty>(std::forward<T>(args) ...)};
		}

		template <std::derived_from<drawable> Ty, typename ...T>
			requires (std::constructible_from<Ty, T...>)
		void set_drawable(const std::size_t idx, T&& ...args){
			this->set_drawable<Ty>(idx, default_style, std::forward<T>(args) ...);
		}

		template <std::derived_from<drawable> Ty, typename ...T>
			requires (std::constructible_from<Ty, T...>)
		void set_drawable(T&& ...args){
			this->set_drawable<Ty>(0, std::forward<T>(args) ...);
		}

		[[nodiscard]] std::size_t get_frame_index() const noexcept{
			return current_frame_index;
		}

	protected:

		void try_swap_image(const std::size_t ldx, const std::size_t rdx){
			if(ldx >= drawables_.size() || rdx >= drawables_.size())return;

			std::swap(drawables_[ldx], drawables_[rdx]);
		}
		// std::unique_ptr<drawable> image_drawable;

		[[nodiscard]] static math::vec2 get_expected_size(const drawable& drawable, const image_display_style& style, const math::vec2 bound) noexcept{
			if(const auto sz = drawable.get_default_size()){
				return align::embedTo(style.scaling, *sz, bound);
			}

			return bound;
		}

		void draw_content(const rect clipSpace) const override{
			auto drawable = get_region();
			if(!drawable || !drawable->drawable)return;
			auto sz = get_expected_size(*drawable->drawable, drawable->style, content_size());
			auto off = align::get_offset_of(default_style.align, sz, property.content_bound_absolute());
			drawable->drawable->draw(*this, rect{tags::from_extent, off, sz}, (drawable->style.color_scl * gprop().style_color_scl).mulA(gprop().get_opacity()), drawable->style.color_mix);
		}

		[[nodiscard]] const styled_drawable* get_region() const noexcept{
			if(current_frame_index < drawables_.size()){
				return &drawables_[current_frame_index];
			}
			return nullptr;
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
