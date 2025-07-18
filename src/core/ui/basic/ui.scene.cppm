module;

export module mo_yanxi.ui.primitives:scene;

import :pre_decl;
import :elem;
import :group;
import :tooltip_manager;
import :dialog_manager;
//
import mo_yanxi.math.vector2;
import mo_yanxi.math.rect_ortho;

export import mo_yanxi.core.ctrl.constants;
export import mo_yanxi.core.ctrl.key_binding;
export import mo_yanxi.ui.flags;

import mo_yanxi.owner;
import mo_yanxi.handle_wrapper;
import mo_yanxi.open_addr_hash_map;
import mo_yanxi.graphic.camera;

//TODO isolate this in future
import mo_yanxi.graphic.renderer.predecl;

import std;

namespace mo_yanxi::ui{
	struct event_channel_manager{
		using subscriber_map = fixed_open_hash_map<event_channel_t, std::vector<elem*>, std::numeric_limits<event_channel_t>::max()>;
	private:
		subscriber_map
			subscribe_elems{};
		fixed_open_hash_map<elem*, std::vector<event_channel_t>, nullptr>
			registery{};

	public:
		bool insert(elem& elem, event_channel_t channel){
			auto& registered_channels = registery[&elem];
			if(auto itr = std::ranges::find(registered_channels, channel); itr == registered_channels.end()){
				registered_channels.push_back(channel);
				subscribe_elems[channel].push_back(&elem);
				return true;
			}else{
				return false;
			}
		}

		std::size_t erase(elem& elem, event_channel_t channel) noexcept{
			if(auto itr = registery.find(&elem); itr != registery.end()){
				auto rst = std::erase(itr->second, channel);
				if(itr->second.empty())registery.erase(itr);

				if(rst){
					auto subscribers = subscribe_elems.find(channel);
					auto count = std::erase(subscribers->second, &elem);
					if(subscribers->second.empty()){
						subscribe_elems.erase(subscribers);
					}
					return count;
				}
			}

			return 0;
		}

		std::size_t erase(elem& elem) noexcept{
			if(auto itr = registery.find(&elem); itr != registery.end()){
				auto count = itr->second.size();
				for (auto channel : itr->second){
					auto subscribers = subscribe_elems.find(channel);
					std::erase(subscribers->second, &elem);

					if(subscribers->second.empty()){
						subscribe_elems.erase(subscribers);
					}
				}
				registery.erase(itr);

				return count;
			}

			return 0;
		}

		std::span<elem*> get_elems_at(event_channel_t channel){
			if(auto rst = subscribe_elems.find(channel); rst != subscribe_elems.end()){
				return rst->second;
			}
			return {};
		}
	};

	export struct scene_base{
		struct MouseState{
			math::vec2 src{};
			bool pressed{};

			void reset(const math::vec2 pos){
				src = pos;
				pressed = true;
			}

			void clear(const math::vec2 pos){

				src = pos;
				pressed = false;
			}
		};

	protected:
		std::string_view name{};
		math::frect region{};
		math::vec2 cursor_pos{};
		std::array<MouseState, core::ctrl::mouse::Count> mouseKeyStates{};

	public:
		bool root_ownership{true};
		dependency<basic_group*> root{};

		elem* currentScrollFocus{nullptr};
		elem* currentCursorFocus{nullptr};
		elem* currentKeyFocus{nullptr};

	protected:
		std::vector<elem*> lastInbounds{};
		std::unordered_set<elem*> independentLayout{};
		std::unordered_set<elem*> asyncTaskOwners{};

	public:
		graphic::renderer_ui_ptr renderer{};
		graphic::camera2* focused_camera{};

		tooltip_manager tooltip_manager{};
		dialog_manager dialog_manager{};

		[[nodiscard]] scene_base() = default;

		[[nodiscard]] scene_base(
			const std::string_view name,
			const owner<basic_group*> root,
			graphic::renderer_ui_ptr renderer = nullptr)
			: name{name},
			  root{root},
			renderer(renderer)
		{}

		scene_base(const scene_base& other) = delete;

		scene_base(scene_base&& other) noexcept = default;

		scene_base& operator=(const scene_base& other) = delete;

		scene_base& operator=(scene_base&& other) noexcept = default;

		[[nodiscard]] std::string_view get_name() const{
			return name;
		}

		[[nodiscard]] math::frect get_region() const noexcept{
			return region;
		}

		[[nodiscard]] math::vec2 get_extent() const noexcept{
			return region.size();
		}

		[[nodiscard]] math::vec2 get_cursor_pos() const noexcept{
			return cursor_pos;
		}

		[[nodiscard]] const basic_group& get_root() const noexcept{
			return *root;
		}

		[[nodiscard]] basic_group& get_root() noexcept{
			return *root;
		}

		[[nodiscard]] std::span<elem* const> get_last_inbounds() const noexcept{
			return lastInbounds;
		}
	};

	export struct scene : scene_base{
		[[nodiscard]] explicit scene(
			std::string_view name,
			owner<basic_group*> root,
			graphic::renderer_ui_ptr renderer = nullptr
		);

		~scene();

		void set_camera_focus(graphic::camera2* camera2) noexcept{
			focused_camera = camera2;
		}

		[[nodiscard]] bool has_camera_focus() const noexcept{
			return focused_camera != nullptr;
		}

		[[nodiscard]] math::vec2 get_cursor_drag_transform(const core::ctrl::key_code_t key) const noexcept{
			return cursor_pos - mouseKeyStates[key].src;
		}

		[[nodiscard]] std::string_view get_clipboard() const noexcept;

		void set_clipboard(const char* sv) const noexcept;

		// void setIMEPos(Geom::Point2 pos) const;

		[[deprecated]] void registerAsyncTaskElement(elem* element);

		void notify_isolated_layout_update(elem* element);

		[[nodiscard]] bool is_mouse_pressed() const noexcept{
			return std::ranges::any_of(mouseKeyStates, std::identity{}, &MouseState::pressed);
		}

		[[nodiscard]] bool is_mouse_pressed(core::ctrl::key_code_t mouse_button_code) const noexcept{
			return mouseKeyStates[mouse_button_code].pressed;
		}

		[[nodiscard]] core::ctrl::key_code_t get_input_mode() const noexcept;

		void drop_dialog(const elem* elem);

		void drop_all_focus(const elem* target);

		void try_swap_focus(elem* newFocus, bool force_drop);

		void swap_focus(elem* newFocus);

		esc_flag on_esc();

		void on_mouse_action(core::ctrl::key_code_t key, core::ctrl::key_code_t action, core::ctrl::key_code_t mode);

		void on_key_action(core::ctrl::key_code_t key, core::ctrl::key_code_t action, core::ctrl::key_code_t mode);

		void on_unicode_input(char32_t val) const;

		void on_scroll(math::vec2 scroll, core::ctrl::key_code_t mode) const;
		void on_scroll(math::vec2 scroll) const{
			on_scroll(scroll, get_input_mode());
		}

		void on_cursor_pos_update(math::vec2 newPos, bool force_drop);
		void on_cursor_pos_update(bool force_drop){
			on_cursor_pos_update(cursor_pos, force_drop);
		}

		void resize(math::frect region);

		void set_position(const math::vec2 pos){
			this->region.src = pos;
		}

		void update(float delta_in_ticks);

		// void setCameraFocus(Graphic::Camera2D* camera) const;

		[[nodiscard]] bool is_cursor_captured() const noexcept;
		[[nodiscard]] elem* getCursorCaptureRoot() const noexcept;

		void layout();

		void root_draw() const;
		void draw(math::frect clipSpace) const;

		[[nodiscard]] bool has_scroll_focus() const noexcept{
			return this->currentScrollFocus != nullptr;
		}

		[[nodiscard]] bool has_key_focus() const noexcept{
			return this->currentKeyFocus != nullptr;
		}

		scene(const scene& other) = delete;

		scene(scene&& other) noexcept;

		scene& operator=(const scene& other) = delete;

		scene& operator=(scene&& other) noexcept;

		void drop_event_focus(const elem* target){
			if(currentCursorFocus == target)currentCursorFocus = nullptr;
			if(currentScrollFocus == target)currentScrollFocus = nullptr;
			if(currentKeyFocus == target)currentKeyFocus = nullptr;
		}

		void swap_event_focus_to_null(){
			try_swap_focus(nullptr, false);
			currentScrollFocus = nullptr;
			currentKeyFocus = nullptr;
		}

		[[nodiscard]] double get_global_time() const noexcept;

	private:
		void updateInbounds(std::vector<elem*>&& next, bool force_drop);

		// void moveOwnerShip();
	};
}
