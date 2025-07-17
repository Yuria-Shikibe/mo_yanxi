//
// Created by Matrix on 2025/3/21.
//

export module mo_yanxi.ui.elem.collapser;

export import mo_yanxi.ui.primitives;

export import mo_yanxi.ui.elem.table;
export import mo_yanxi.ui.elem.button;
export import mo_yanxi.math.interpolation;

import std;

namespace mo_yanxi::ui{
	//TODO support vert_major layout

	export
	struct collapser : basic_group{
		using head_type = button<table>;
		using content_type = table;
	private:
		head_type* head_;
		content_type* content_;
		layout_policy policy{layout_policy::hori_major};

		float pad = 8;

		bool expanded{};
		float expand_progress{};

		void set_fillparent() const{
			switch(policy){
			case layout_policy::hori_major:
				group::set_fillparent(*head_, content_size(), {true, false}, false);
				group::set_fillparent(*content_, content_size(), {true, false}, false);
				break;
			case layout_policy::vert_major:

				group::set_fillparent(*head_, content_size(), {false, true}, false);
				group::set_fillparent(*content_, content_size(), {false, true}, false);
				break;
			default:
				break;
			}
		}

		void update_item_src() const{
			head_->update_abs_src(content_src_pos());
			content_->update_abs_src(content_src_pos().add_y(head_->property.get_size().y + pad));
		}

		void collapse(const input_event::click e){
			if(e.code.action() != core::ctrl::act::release)return;

			expanded = !expanded;
			fire_event(input_event::collapser_state_changed{expanded}, spread_direction::child, true);
		}

	public:
		float expand_speed{3.75f / 60.f};

		collapser(collapser&& other) noexcept = delete;
		collapser& operator=(collapser&& other) noexcept = delete;

		[[nodiscard]] collapser(scene* scene, group* group)
			: basic_group(scene, group, "collapser"),
			  head_(&static_cast<head_type&>(basic_group::add_children(elem_ptr{get_scene(), this, std::in_place_type<head_type>}))),
			  content_(&static_cast<content_type&>(basic_group::add_children(elem_ptr{
				                                                                 get_scene(), this, std::in_place_type<content_type>
			                                                                 }))){
			head().property.fill_parent = {true, true};
			head().interactivity = interactivity::intercepted;

			content().property.fill_parent = {true, true};
			content().property.set_empty_drawer();

			set_fillparent();

			head().set_button_callback(button_tags::general, [](const input_event::click e, const head_type& self){
				static_cast<collapser&>(*self.get_parent()).collapse(e);
			});

		}

		auto get_collapse_func() noexcept{
			return [this](const input_event::click e){
				collapse(e);
			};
		}


		// void set_pad(){
		//
		// }

		[[nodiscard]] head_type& head() const noexcept{
			return *head_;
		}

		[[nodiscard]] content_type& content() const noexcept{
			return *content_;
		}

		std::optional<math::vec2> pre_acquire_size_impl(optional_mastering_extent extent) override{
			auto ext = clip_boarder_from(extent);
			auto table_size = head_->pre_acquire_size(ext);

			if(policy == layout_policy::hori_major){
				extent.set_height_dependent();

				math::vec2 content_size{};
				if(get_interped_progress() >= std::numeric_limits<float>::epsilon()){
					content_size.lerp_inplace(content_->pre_acquire_size(ext).value_or(content_->get_size()).add_y(pad), get_interped_progress());
				}
				return table_size.value().add_y(content_size.y) + property.boarder.extent();
			}else{
				return std::nullopt;
			}

		}

		bool update_abs_src(math::vec2 parent_content_abs_src) override{
			if(util::try_modify(property.absolute_src, parent_content_abs_src + property.relative_src)){
				update_item_src();
				return true;
			}
			return false;
		}

	protected:
		void draw_pre(const rect clipSpace) const override;

		void draw_content(const rect clipSpace) const override;

		void layout() override{
			elem::layout();
			set_fillparent();

			head_->try_layout();
			content_->try_layout();
			update_item_src();
		}


		void update(float delta_in_ticks) override{
			basic_group::update(delta_in_ticks);

			auto prog = expand_progress;

			if(expanded){
				math::approach_inplace(prog, 1, expand_speed * delta_in_ticks);
				content_->visible = true;
			}else{
				math::approach_inplace(prog, 0, expand_speed * delta_in_ticks);
				if(expand_progress < std::numeric_limits<float>::epsilon()){
					content_->visible = false;
				}
			}

			if(util::try_modify(expand_progress, prog)){
				notify_layout_changed(spread_direction::from_content | spread_direction::local);
				content_->update_opacity(get_interped_progress());

			}

		}

		[[nodiscard]] float get_interped_progress() const noexcept{
			return math::interp::smooth(expand_progress);
		}

		[[nodiscard]] bool is_expanding() const noexcept{
			return !math::equal(expand_progress, 1.f) && !math::equal(expand_progress, 0.f);
		}

		[[nodiscard]] rect get_collapsed_region() const noexcept{
			auto height = head_->get_size().y + pad;
			return rect{tags::from_extent, content_src_pos().add_y(height), content_size().add_y(-height).max_y(0)};
		}
	};
}
