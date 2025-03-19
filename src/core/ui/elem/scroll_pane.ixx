//
// Created by Matrix on 2025/3/15.
//

export module mo_yanxi.ui.scroll_pane;

export import mo_yanxi.ui.elem;
export import mo_yanxi.ui.group;
export import mo_yanxi.ui.elem_ptr;
export import mo_yanxi.ui.layout.policies;

import mo_yanxi.snap_shot;
import std;

namespace mo_yanxi::ui{
	export
	struct scroll_pane final : group{

		static constexpr float VelocitySensitivity = 0.95f;
		static constexpr float VelocityDragSensitivity = 0.15f;
		static constexpr float VelocityScale = 20.f;

	private:
		float scrollBarStroke{20.0f};

		math::vec2 scrollVelocity{};
		math::vec2 scrollTargetVelocity{};
		snap_shot<math::vec2> scroll{};

		elem_ptr item{};
		layout_policy layout_policy_{layout_policy::horizontal};
		bool bar_caps_size{true};

	public:

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

		[[nodiscard]] scroll_pane(scene* scene, group* group)
			: group(scene, group, "scroll_pane"){

			events().on<events::drag>([](const events::drag& e, elem& el){
				auto& self = static_cast<scroll_pane&>(el);

				self.scrollTargetVelocity = self.scrollVelocity = {};
				const auto trans = e.trans() * self.get_vel_clamp();
				const auto blank = self.get_viewport_size() - math::vec2{self.bar_hori_length(), self.bar_vert_length()};

				auto rst = self.scroll.base + (trans / blank) * self.get_scrollable_size();

				//clear NaN
				if(!self.enable_hori_scroll())rst.x = 0;
				if(!self.enable_vert_scroll())rst.y = 0;

				rst.clamp_xy({}, self.get_scrollable_size());

				if(util::tryModify(self.scroll.temp, rst)){
					self.updateChildrenAbsSrc();
				}
			});

			events().on<events::click>([](const events::click e, elem& el){
				auto& self = static_cast<scroll_pane&>(el);

				if(e.code.action() == core::ctrl::act::release){
					self.scroll.apply();
				}
			});
			//
			events().on<events::scroll>([](const events::scroll& e, elem& el){
				auto& self = static_cast<scroll_pane&>(el);

				auto cmp = -e.pos;

				if(e.mode & core::ctrl::mode::Shift){
					cmp.swap_xy();
				}

				self.scrollTargetVelocity = cmp * self.get_vel_clamp();

				self.scrollVelocity = self.scrollTargetVelocity.scl(VelocityScale);
			});
			//
			events().on<events::inbound>([](const auto& e, elem& el){
				el.set_focused_scroll(true);
			});

			events().on<events::exbound>([](const auto& e, elem& el){
				el.set_focused_scroll(false);
			});

			property.maintain_focus_until_mouse_drop = true;
		}

		void update(const float delta_in_ticks) override{
			elem::update(delta_in_ticks);


			{//scroll update
				scrollVelocity.lerp(scrollTargetVelocity, delta_in_ticks * VelocitySensitivity);
				scrollTargetVelocity.lerp({}, delta_in_ticks * VelocityDragSensitivity);


				if(util::tryModify(
					scroll.base,
						scroll.base.copy().fma(scrollVelocity, delta_in_ticks).clamp_xy({}, get_scrollable_size()) * get_vel_clamp())){
					scroll.resume();
					updateChildrenAbsSrc();
				}
			}

			// prevRatio = getScrollRatio(scroll.base);

			item->update(delta_in_ticks);
		}

		void draw_content(const rect clipSpace, rect redirect) const override{
			if(item)item->draw(clipSpace, redirect);
		}

		void draw_pre(rect clipSpace, rect redirect) const override;

		void draw_post(rect clipSpace, rect redirect) const override;

		void layout() override{
			group::layout();
		}

		template <math::vector2<bool> fillParent = {true, false}, elem_init_func Fn>
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

	private:
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

		void layout_children() override{
			update_item_layout();
		}

		bool resize(const math::vec2 size) override{
			if(elem::resize(size)){
				update_item_layout();

				return true;
			}

			return false;
		}

		void update_item_layout(){
			if(item){
				modifyChildren(*item);
				setChildrenFillParentSize_Quiet(*item, content_size());

				auto bound = item->context_size_restriction;

				if(auto sz = item->pre_acquire_size(bound)){

					if(bar_caps_size){
						bool need_relayout = false;
						switch(layout_policy_){
						case layout_policy::horizontal :{
							if(sz->y > property.content_height()){
								bound.width.value -= scrollBarStroke;
								bound.width.value = math::clamp_positive(bound.width.value);
								need_relayout = true;
							}
							break;
						}
						case layout_policy::vertical :{
							if(sz->x > property.content_width()){
								bound.height.value -= scrollBarStroke;
								bound.height.value = math::clamp_positive(bound.height.value);
								need_relayout = true;
							}
							break;
						}
						default: break;
						}

						if(need_relayout){
							auto s = item->pre_acquire_size(bound);
							if(s) sz = s;
						}
					}

					item->resize_quiet(*sz, spread_direction::local | spread_direction::child);
				}

				// setChildrenFillParentSize_Quiet(*item, get_viewport_size());
				item->layout();
			}
		}

		void modifyChildren(elem& element) const{
			element.context_size_restriction = extent_by_external;
			switch(layout_policy_){
			case layout_policy::horizontal :{
				element.property.fill_parent = {true, false};
				element.context_size_restriction.width = {size_category::mastering, property.content_width()};
				break;
			}
			case layout_policy::vertical :{
				element.property.fill_parent = {false, true};
				element.context_size_restriction.height = {size_category::mastering, property.content_height()};
				break;
			}
			case layout_policy::none:
				element.property.fill_parent = {false, false};
				break;
			default: std::unreachable();
			}
		}

		void postRemove(elem* element) override{
			item = {};
		}

		void instantRemove(elem* element) override{
			item = {};
		}

		void clearChildren() noexcept override{
			item = {};
		}
		elem& addChildren(elem_ptr&& element) override{
			return *item;
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

			if(enable_hori_scroll())rst.y += scrollBarStroke;
			if(enable_vert_scroll())rst.x += scrollBarStroke;

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