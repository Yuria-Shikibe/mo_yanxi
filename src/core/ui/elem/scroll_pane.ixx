//
// Created by Matrix on 2025/3/15.
//

export module mo_yanxi.ui.elem.scroll_pane;

export import mo_yanxi.ui.primitives;
export import mo_yanxi.ui.layout.policies;

import mo_yanxi.snap_shot;
import std;

namespace mo_yanxi::ui{
	export
	struct scroll_pane final : group{

		static constexpr float VelocitySensitivity = 0.95f;
		static constexpr float VelocityDragSensitivity = 0.15f;
		static constexpr float VelocityScale = 55.f;

	private:
		float scroll_bar_stroke_{20.0f};

		math::vec2 scrollVelocity{};
		math::vec2 scrollTargetVelocity{};
		snap_shot<math::vec2> scroll{};

		elem_ptr item{};
		layout_policy layout_policy_{layout_policy::hori_major};
		bool bar_caps_size{true};

	public:

		template <std::derived_from<elem> T = elem>
		[[nodiscard]] T& get_item() const noexcept{
			return dynamic_cast<T&>(*item);
		}

		template <std::derived_from<elem> T = elem>
		[[nodiscard]] T& get_item_unchecked() const noexcept{
			return static_cast<T&>(*item);
		}

		[[nodiscard]] std::span<const elem_ptr> get_children() const noexcept override{
			return item ? std::span{&item, 1} : std::span{static_cast<const elem_ptr*>(nullptr), 0};
		}

		[[nodiscard]] bool contains_parent(math::vec2 cursorPos) const override{
			return elem::contains_parent(cursorPos) && get_viewport().contains_loose(cursorPos);
		}

		// [[nodiscard]] bool contains_self(math::vec2 absPos, float margin) const noexcept override{
		// 	rect region{tags::from_extent, abs_pos(), get_viewport_size() + property.content_src_offset()};
		// 	return property.content_bound_absolute().expand(margin, margin).contains_loose(absPos) && !region.expand(margin, margin).contains_loose(absPos);
		// }

		[[nodiscard]] scroll_pane(scene* scene, group* group_)
			: group(scene, group_, "scroll_pane"){

			events().on<input_event::inbound>([](const auto& e, elem& el){
				el.set_focused_scroll(true);
			});

			events().on<input_event::exbound>([](const auto& e, elem& el){
				el.set_focused_scroll(false);
			});

			property.maintain_focus_until_mouse_drop = true;
		}

	protected:
		void update(const float delta_in_ticks) override{
			elem::update(delta_in_ticks);


			{//scroll update
				scrollVelocity.lerp_inplace(scrollTargetVelocity, delta_in_ticks * VelocitySensitivity);
				scrollTargetVelocity.lerp_inplace({}, delta_in_ticks * VelocityDragSensitivity);


				if(util::try_modify(
					scroll.base,
						scroll.base.fma(scrollVelocity, delta_in_ticks).clamp_xy({}, get_scrollable_size()) * get_vel_clamp())){
					scroll.resume();
					updateChildrenAbsSrc();
				}
			}

			// prevRatio = getScrollRatio(scroll.base);

			if(item)item->update(delta_in_ticks);
		}

		void draw_content(const rect clipSpace) const override{
			if(item)item->draw(clipSpace);
		}

		void draw_pre(rect clipSpace) const override;

		void draw_post(rect clipSpace) const override;

		void layout() override{
			elem::layout();
			update_item_layout();
		}

		void on_drag(const input_event::drag e) override{
			auto& self = *this;

			self.scrollTargetVelocity = self.scrollVelocity = {};
			const auto trans = e.trans() * self.get_vel_clamp();
			const auto blank = self.get_viewport_size() - math::vec2{self.bar_hori_length(), self.bar_vert_length()};

			auto rst = self.scroll.base + (trans / blank) * self.get_scrollable_size();

			//clear NaN
			if(!self.enable_hori_scroll())rst.x = 0;
			if(!self.enable_vert_scroll())rst.y = 0;

			rst.clamp_xy({}, self.get_scrollable_size());

			if(util::try_modify(self.scroll.temp, rst)){
				self.updateChildrenAbsSrc();
			}
		}
	public:

		template </*math::vector2<bool> fillParent = {true, false}, */elem_init_func Fn>
		typename elem_init_func_trait<Fn>::elem_type& set_elem(Fn&& init){
			this->item = elem_ptr{get_scene(), this, [&](typename elem_init_func_trait<Fn>::elem_type& e){

				scroll_pane::modifyChildren(e);

				init(e);
			}};
			updateChildrenAbsSrc();
			// setItemSize();

			return static_cast<typename elem_init_func_trait<Fn>::elem_type&>(*this->item);
		}
		template <std::derived_from<elem> E>
		E& emplace(){
			this->item = elem_ptr{get_scene(), this, std::in_place_type<E>};
			modifyChildren(*this->item);
			updateChildrenAbsSrc();
			return static_cast<E&>(*this->item);
		}

		void set_layout_policy(layout_policy policy){
			if(layout_policy_ != policy){
				layout_policy_ = policy;
				update_item_layout();
			}
		}


		bool resize(const math::vec2 size) override{
			if(elem::resize(size)){
				update_item_layout();

				return true;
			}

			return false;
		}

		[[nodiscard]] float get_scroll_bar_stroke() const{
			return scroll_bar_stroke_;
		}

		void set_scroll_bar_stroke(const float scroll_bar_stroke){
			scroll_bar_stroke_ = scroll_bar_stroke;
			notify_isolated_layout_changed();
		}

	private:
		void on_scroll(const input_event::scroll e) override{

			auto cmp = -e.delta;

			if((e.mode & core::ctrl::mode::shift) || (enable_hori_scroll() && !enable_vert_scroll())){
				cmp.swap_xy();
			}

			scrollTargetVelocity = cmp * get_vel_clamp();

			scrollVelocity = scrollTargetVelocity.scl(VelocityScale);
		}

		input_event::click_result on_click(const input_event::click click_event) override{
			if(click_event.code.action() == core::ctrl::act::release){
				scroll.apply();
			}

			return elem::on_click(click_event);
		}
		// [[nodiscard]] math::vec2 get_bound() const noexcept{
		// 	switch(layout_policy_){
		// 	case layout_policy::horizontal :
		// 		return {property.content_width(), std::numeric_limits<float>::infinity()};
		// 	case layout_policy::vertical :
		// 		return {std::numeric_limits<float>::infinity(), property.content_height()};
		// 	case layout_policy::none:
		// 		return math::vectors::constant2<float>::inf_positive_vec2;
		// 	default: std::unreachable();
		// 	}
		// }

		void update_item_layout();

		void modifyChildren(elem& element) const{
			element.context_size_restriction = extent_by_external;
			switch(layout_policy_){
			case layout_policy::hori_major :{
				element.property.fill_parent = {true, false};
				element.context_size_restriction.set_width(property.content_width());
				break;
			}
			case layout_policy::vert_major :{
				element.property.fill_parent = {false, true};
				element.context_size_restriction.set_height(property.content_height());
				break;
			}
			case layout_policy::none:
				element.property.fill_parent = {false, false};
				break;
			default: std::unreachable();
			}
		}

	private:
		void set_scroll_by_ratio(math::vec2 ratio){

		}

		void updateChildrenAbsSrc() const{
			if(!item)return;
			item->property.relative_src = -scroll.temp;
			item->update_abs_src(content_src_pos());
		}

		[[nodiscard]] constexpr math::vec2 get_vel_clamp() const noexcept{
			return math::vector2{enable_hori_scroll(), enable_vert_scroll()}.as<float>();
		}

		[[nodiscard]] constexpr bool enable_hori_scroll() const noexcept{
			return get_item_size().x > property.content_width();
		}

		[[nodiscard]] constexpr bool enable_vert_scroll() const noexcept{
			return get_item_size().y > property.content_height();
		}

		[[nodiscard]] constexpr math::vec2 get_scrollable_size() const noexcept{
			return (get_item_size() - get_viewport_size()).max({});
		}

		[[nodiscard]] constexpr math::nor_vec2 get_scroll_progress(const math::vec2 scroll_pos) const noexcept{
			return scroll_pos / get_scrollable_size();
		}

		[[nodiscard]] constexpr math::vec2 get_item_size() const noexcept{
			return item ? item->get_size() : math::vec2{};
		}

		[[nodiscard]] constexpr math::vec2 get_bar_size() const noexcept{
			math::vec2 rst{};

			if(enable_hori_scroll())rst.y += scroll_bar_stroke_;
			if(enable_vert_scroll())rst.x += scroll_bar_stroke_;

			return rst;
		}

		[[nodiscard]] float bar_hori_length() const {
			const auto w = get_viewport_size().x;
			return math::clamp_positive(math::min(w / get_item_size().x, 1.0f) * w);
		}

		[[nodiscard]] float bar_vert_length() const {
			const auto h = get_viewport_size().y;
			return math::clamp_positive(math::min(h / get_item_size().y, 1.0f) * h);
		}


		[[nodiscard]] constexpr math::vec2 get_viewport_size() const noexcept{
			return content_size() - get_bar_size();
		}

		[[nodiscard]] constexpr rect get_viewport() const noexcept{
			return {tags::from_extent, content_src_pos(), get_viewport_size()};
		}

		[[nodiscard]] rect get_hori_bar_rect() const {
			const auto [x, y] = content_src_pos();
			const auto ratio = get_scroll_progress(scroll.temp);
			const auto barSize = get_bar_size();
			const auto width = bar_hori_length();
			return {
				x + ratio.x * (property.content_width() - barSize.x - width),
				y - barSize.y + property.content_height(),
				width,
				barSize.y
			};
		}

		[[nodiscard]] rect get_vert_bar_rect() const {
			const auto [x, y] = content_src_pos();
			const auto ratio = get_scroll_progress(scroll.temp);
			const auto barSize = get_bar_size();
			const auto height = bar_vert_length();
			return {
				x - barSize.x + property.content_width(),
				y + ratio.y * (property.content_height() - barSize.y - height),
				barSize.x,
				height
			};
		}

		[[nodiscard]] float hori_scroll_ratio(const float pos) const {
			return math::clamp(pos / (get_item_size().x - get_viewport_size().x));
		}

		[[nodiscard]] float vert_scroll_ratio(const float pos) const {
			return math::clamp(pos / (get_item_size().y - get_viewport_size().y));
		}

		[[nodiscard]] math::vec2 get_scroll_ratio(const math::vec2 pos) const {
			auto [ix, iy] = get_item_size();
			auto [vx, vy] = get_viewport_size();

			return {
				enable_hori_scroll() ? math::clamp(pos.x / (ix - vx)) : 0.f,
				enable_vert_scroll() ? math::clamp(pos.y / (iy - vy)) : 0.f,
			};
		}
	};
}