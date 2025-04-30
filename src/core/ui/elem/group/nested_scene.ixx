module;

#include <cassert>

export module mo_yanxi.ui.elem.nested_scene;

export import mo_yanxi.ui.basic;
export import mo_yanxi.graphic.camera;
export import mo_yanxi.math.quad_tree;

import std;

namespace mo_yanxi::ui{
	export
	struct elem_wrapper{
		virtual ~elem_wrapper() = default;

		virtual void draw(elem& wrapped){
			// using namespace Graphic;
			// AutoParam auto_param{wrapped.getBatch(), Draw::WhiteRegion};
			//
			// Drawer::Line::rectOrtho(auto_param, 4, wrapped.prop().getBound_absolute(), Colors::WHITE);
		}

		virtual void draw_pre(elem& wrapped){
			// using namespace Graphic;
			// AutoParam auto_param{wrapped.getBatch(), Draw::WhiteRegion};
			//
			// Drawer::Line::rectOrtho(auto_param, 4, wrapped.prop().getBound_absolute(), Colors::WHITE);
		}

		virtual void draw_post(elem& wrapped){
			// using namespace Graphic;
			// AutoParam auto_param{wrapped.getBatch(), Draw::WhiteRegion};
			//
			// Drawer::Line::rectOrtho(auto_param, 4, wrapped.prop().getBound_absolute(), Colors::WHITE);
		}

		virtual void notifyRemove(elem& wrapped){

		}
	};
	
	struct nested_scene_group : basic_group{

		[[nodiscard]] nested_scene_group(scene* scene, group* group)
			: basic_group(scene, group, "nested_scene_group"){
		}

		// elem* manager{};
		//TODO element layout may cause the quad tree disable
		math::quad_tree<elem> quadTree{rect{math::vec2{}, 10000}};
		// fixed_open_addr_hash_map<elem*, std::unique_ptr<ElementController>, elem*, nullptr>
		// 	controllers{};

		std::unordered_set<elem*> selected{};
		std::optional<elem*> mainSelected{};

		// bool linkController(elem* element, std::unique_ptr<elem_wrapper>&& controller){
		// 	if(std::ranges::contains(children, element, &ElementUniquePtr::get)){
		// 		auto [itr, suc] = controllers.try_emplace(element, std::move(controller));
		// 		return suc;
		// 	}
		//
		// 	throw std::invalid_argument("element not found");
		// }

		template <invocable_elem_init_func Fn>
		typename elem_init_func_trait<Fn>::elem_type& function_init(Fn init){
			auto& rst = add<typename elem_init_func_trait<Fn>::elem_type>(elem_ptr{get_scene(), this, init});
			static_cast<elem&>(rst).context_size_restriction = extent_by_external;
			return rst;
		}
		template <typename T>
		T& emplace(){
			T& rst = add<T>(elem_ptr{get_scene(), this, std::in_place_type<T>});
			static_cast<elem&>(rst).context_size_restriction = extent_by_external;
			return rst;
		}

		[[nodiscard]] elem* try_find(const math::vec2 cursor, const float hitboxSize) const{
			const rect point{cursor, hitboxSize};
			if(quadTree.get_boundary().overlap_exclusive(point)){
				return quadTree.intersect_any(point);
			} else{
				for(const auto& element : children){
					if(element->get_bound().overlap_exclusive(point)){
						return element.get();
					}
				}
			}

			return nullptr;
		}

		// void drawSelected(rect clipSpace) const{
		// 	AutoParam auto_param{getBatch(), Graphic::Draw::WhiteRegion};
		// 	for (const auto & element : selected){
		// 		Drawer::Line::rectOrtho(auto_param, 3, element->getBound(), Graphic::Colors::LIGHT_GRAY);
		// 	}
		//
		// 	if(mainSelected){
		// 		Drawer::Line::rectOrtho(auto_param, 3, mainSelected.value()->getBound(), Graphic::Colors::ACID);
		// 	}
		// }
		//
		// void drawMain(const rect clipSpace) const override{
		// 	for (const auto & [elem, ctrl] : controllers){
		// 		ctrl->drawPre(*elem);
		// 	}
		//
		// 	for(const auto& element : getChildren()){
		// 		element->tryDraw(clipSpace);
		// 		if(auto itr = controllers.find(element.get()); itr != controllers.end()){
		// 			itr->second->draw(*element);
		// 		}
		// 	}
		//
		// 	for (const auto & [elem, ctrl] : controllers){
		// 		ctrl->drawPost(*elem);
		// 	}
		// 	drawSelected(clipSpace);
		//
		// }

		elem& add_children(elem_ptr&& element, std::size_t where) override{
			quadTree.insert(*element);
			// element->update_abs_src({});
			return basic_group::add_children(std::move(element), where);
		}

		using basic_group::add_children;

		// friend Pool;

	private:
		template <std::derived_from<elem> E = elem>
		E& add(elem_ptr&& ptr){
			return static_cast<E&>(add_children(std::move(ptr)));
		}
	public:

		void update_elem_quad_tree(elem& elem){
			if(quadTree.erase(elem)){
				quadTree.insert(elem);
			}
		}

		// void selectMain(elem* element, const bool append) {
		// 	if(!element){
		// 		mainSelected.reset();
		// 		selected.clear();
		// 		return;
		// 	}
		//
		// 	if(std::ranges::contains(children, element, &ElementUniquePtr::get)){
		// 		if(mainSelected && append){
		// 			selected.insert(*mainSelected);
		// 		}
		// 		selected.erase(element);
		// 		mainSelected.emplace(element);
		// 		return;
		// 	}
		//
		// 	//TODO throw??
		// 	mainSelected.reset();
		// 	selected.clear();
		// }
		//
		// void selectMulti(const rect local_region) {
		// 	quadTree.intersect_then(local_region, [this](elem& element, rect){
		// 		selected.emplace(&element);
		// 	});
		// }

		// void dropSelection(elem* element){
		// 	if(mainSelected == element){
		// 		mainSelected.reset();
		// 	}
		//
		// 	selected.erase(element);
		// }

		void clear_children() noexcept override{
			// for (const auto & [elem, ctrl] : controllers){
			// 	ctrl->notifyRemove(*elem);
			// }
			selected.clear();
			mainSelected.reset();
			// controllers.clear();
			quadTree.clear();
			basic_group::clear_children();
		}

		void post_remove(elem* element) override{
			if(const auto itr = find(element); itr != children.end()){
				removeElemProperty(*element);
				toRemove.push_back(std::move(*itr));
				children.erase(itr);
			}

			notify_layout_changed(spread_direction::all_visible);
		}

		void instant_remove(elem* element) override{
			if(const auto itr = find(element); itr != children.end()){
				removeElemProperty(*element);
				children.erase(itr);
			}

			notify_layout_changed(spread_direction::all_visible);
		}

		// void selectionHint(const Event::ElementConnection::CodeType code){
		// 	if(mainSelected){
		// 		mainSelected.value()->events().fire(Event::ElementConnection{selected, code});
		// 	}
		// }
	protected:
		void layout_children() override{
			for(const auto& element : children){
				element->update_abs_src({});

				rect region = element->get_bound();
				element->try_layout();
				if(region != element->get_bound()){
					quadTree.erase(*element);
					quadTree.insert(*element);
				}
			}
		}

		// void notify_layout_changed(spread_direction toDirection) override{
		//
		// }
		// //TODO manager self update instead of notify here?
		// void notifyLayoutChanged(const SpreadDirection toDirection) override{
		// 	if(toDirection & SpreadDirection::super || toDirection & SpreadDirection::super_force){
		// 		manager->notifyLayoutChanged(toDirection);
		// 	}
		// }

		void removeElemProperty(elem& element){
			// if(const auto controllerItr = controllers.find(&element); controllerItr != controllers.end()){
			// 	controllerItr->second->notifyRemove(element);
			// 	controllers.erase(controllerItr);
			// }else{
			// 	throw std::logic_error("element not found");
			// }

			if(mainSelected == &element){
				mainSelected.reset();
			}

			selected.erase(&element);
			quadTree.erase(element);
		}
	};

	struct drag_state{
		elem* element{};
		math::vec2 src_pos{};
		math::vec2 offset{};

		[[nodiscard]] drag_state() = default;

		[[nodiscard]] drag_state(elem& element, const math::vec2 src_pos)
			: element(&element),
			  src_pos(src_pos), offset(element.abs_pos() - src_pos){
		}

		void drop(nested_scene_group& group, math::vec2 pos) const{
			assert(element);

			element->property.relative_src = pos + offset;
			element->update_abs_src({});
			group.update_elem_quad_tree(*element);
		}
	};

	export
	struct nested_scene : elem{
		using group_type = nested_scene_group;
		[[nodiscard]] nested_scene(scene* scene, group* group)
			: elem(scene, group), group_(scene, group), scene_("nested_scene", &group_, scene->renderer){
			scene_.root_ownership = false;
			group_.property.set_empty_drawer();

			camera_.flip_y = true;

			register_event([](events::cursor_moved e, nested_scene& self){
				auto p = self.getTransferredPos(self.get_scene()->cursor_pos);
				self.scene_.on_cursor_pos_update(p);
			});

			register_event([](events::focus_begin e, nested_scene& self){
				self.get_scene()->set_camera_focus(&self.camera_);
				self.set_focused_scroll(true);
			});

			register_event([](events::focus_end e, nested_scene& self){
				self.get_scene()->set_camera_focus(nullptr);
				self.set_focused_scroll(false);
			});

			register_event([](events::scroll e, nested_scene& self){

				if(self.scene_.has_scroll_focus()){
					self.scene_.on_scroll(e.pos);
				}else{
					self.camera_.set_scale_by_delta(e.pos.y * 0.05f);
				}

			});
		}

		[[nodiscard]] group_type& get_group(){
			return group_;
		}

	protected:
		graphic::camera2 camera_{};
		group_type group_;
		scene scene_;

		std::optional<drag_state> drag_state_{};

		void draw_pre(rect clipSpace) const override;

		void draw_content(rect clipSpace) const override;

		void draw_post(rect clipSpace) const override;

		void update(float delta_in_ticks) override{
			// scene_.resize(property.content_bound_absolute().move_x(math::absin(get_scene()->get_global_time(), 10, property.content_width())));

			elem::update(delta_in_ticks);
			camera_.update(delta_in_ticks);

			scene_.update(delta_in_ticks);
			scene_.layout();

			if(scene_.has_key_focus()){
				set_focused_key(true);
				camera_.speed_scale = 0;
			}else{
				camera_.speed_scale = 1;
			}
		}

		bool resize(const math::vec2 size) override{
			if(elem::resize(size)){
				scene_.resize(property.content_bound_absolute());
				auto [x, y] = scene_.region.size();
				camera_.resize_screen(x, y);
				return true;
			}
			return false;
		}

		events::click_result on_click(const events::click click_event) override{
			elem::on_click(click_event);

			auto [k, a, m] = click_event.unpack();

			using namespace core::ctrl;


			if(drag_state_.has_value()){
				if(a != act::release)return events::click_result::intercepted;

				auto cpos = getTransferredPos(click_event.pos);
				drag_state_->drop(group_, cpos);
				drag_state_ = std::nullopt;
			} else{
				if(a != act::press) goto pass;

				auto cpos = getTransferredPos(click_event.pos);
				auto elem = group_.try_find(cpos, 8);
				if(elem && group_.try_find(cpos, 1) == elem){
					elem = nullptr;
				}

				if(elem){
					drag_state_.emplace(*elem, cpos);
				}
			}

			pass:

			if(!drag_state_.has_value())scene_.on_mouse_action(k, a, m);

			return events::click_result::intercepted;
		}

		void input_key(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) override{
			scene_.on_key_action(key, action, mode);
		}

		[[nodiscard]] math::vec2 getTransferredPos(const math::vec2 pos) const{
			return camera_.get_screen_to_world(pos, content_src_pos());
		}

		elem* get_selection(math::vec2 pos) const{
			auto elem = group_.try_find(pos, 12);
			if(elem && group_.try_find(pos, 0.5f) == elem){
				elem = nullptr;
			}

			return elem;
		}

		void input_unicode(const char32_t val) override{
			scene_.on_unicode_input(val);
		}
	};
}
