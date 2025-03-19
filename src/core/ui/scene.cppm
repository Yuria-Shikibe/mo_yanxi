//
// Created by Matrix on 2024/10/1.
//

export module mo_yanxi.ui.scene;

export import mo_yanxi.ui.tooltip_manager;

import mo_yanxi.math.vector2;
import mo_yanxi.math.rect_ortho;

export import mo_yanxi.core.ctrl.constants;
export import mo_yanxi.core.ctrl.key_binding;
export import mo_yanxi.ui.flags;
export import mo_yanxi.ui.pre_decl;

import mo_yanxi.owner;
import mo_yanxi.handle_wrapper;

import std;

namespace mo_yanxi{
	namespace graphic{
		export struct renderer_ui;
	}

}


namespace mo_yanxi::ui{
	// struct elem;
	
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

		std::string_view name{};

		math::frect region{};

		math::vec2 cursorPos{};
		std::array<MouseState, core::ctrl::mouse::Count> mouseKeyStates{};



		dependency<owner<group*>> root{};

		// dependency<core::ctrl::key_mapping<>*> keyMapping{};
		//focus fields
		// struct elem* currentInputFocused{nullptr};
		elem* currentScrollFocus{nullptr};
		elem* currentCursorFocus{nullptr};
		elem* currentKeyFocus{nullptr};

		std::vector<elem*> lastInbounds{};
		std::unordered_set<elem*> independentLayout{};
		std::unordered_set<elem*> asyncTaskOwners{};

		graphic::renderer_ui* renderer{};

		tooltip_manager tooltip_manager{};

		[[nodiscard]] scene_base() = default;

		[[nodiscard]] scene_base(
			const std::string_view name,
			const owner<group*> root,
			graphic::renderer_ui* renderer = nullptr)
			: name{name},
			  root{root},
			renderer(renderer)
		{}

		scene_base(const scene_base& other) = delete;

		scene_base(scene_base&& other) noexcept = default;

		scene_base& operator=(const scene_base& other) = delete;

		scene_base& operator=(scene_base&& other) noexcept = default;
	};


	export struct scene : scene_base{
		// ToolTipManager tooltipManager{};

		[[nodiscard]] explicit scene(
			std::string_view name,
			owner<group*> root,
			graphic::renderer_ui* renderer = nullptr
		);

		~scene();

		// [[nodiscard]] math::frect getBound() const noexcept{
		// 	return math::frect{pos, size};
		// }

		[[nodiscard]] math::vec2 getCursorDragTransform(const core::ctrl::key_code_t key) const noexcept{
			return cursorPos - mouseKeyStates[key].src;
		}

		[[nodiscard]] std::string_view get_clipboard() const noexcept;

		void set_clipboard(const char* sv) const noexcept;

		// void setIMEPos(Geom::Point2 pos) const;

		void registerAsyncTaskElement(elem* element);

		void notifyLayoutUpdate(elem* element);

		[[nodiscard]] bool isMousePressed() const noexcept{
			return std::ranges::any_of(mouseKeyStates, std::identity{}, &MouseState::pressed);
		}

		// void joinTasks();

		[[nodiscard]] core::ctrl::key_code_t get_input_mode() const noexcept;

		void dropAllFocus(const elem* target);

		void trySwapFocus(elem* newFocus);

		void swapFocus(elem* newFocus);

		esc_flag on_esc();

		void on_mouse_action(core::ctrl::key_code_t key, core::ctrl::key_code_t action, core::ctrl::key_code_t mode);

		void on_key_action(core::ctrl::key_code_t key, core::ctrl::key_code_t action, core::ctrl::key_code_t mode);

		void on_unicode_input(char32_t val) const;

		void on_scroll(math::vec2 scroll) const;

		void on_cursor_pos_update(math::vec2 newPos);
		void on_cursor_pos_update(){
			on_cursor_pos_update(cursorPos);
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

		void draw() const;
		void draw(math::frect clipSpace) const;

		[[nodiscard]] bool hasScrollFocus() const noexcept{
			return this->currentScrollFocus != nullptr;
		}

		[[nodiscard]] bool hasKeyFocus() const noexcept{
			return this->currentKeyFocus != nullptr;
		}

		scene(const scene& other) = delete;

		scene(scene&& other) noexcept;

		scene& operator=(const scene& other) = delete;

		scene& operator=(scene&& other) noexcept;

		void dropEventFocus(const elem* target){
			if(currentCursorFocus == target)currentCursorFocus = nullptr;
			if(currentScrollFocus == target)currentScrollFocus = nullptr;
			if(currentKeyFocus == target)currentKeyFocus = nullptr;
		}

		double getGlobalTime() const noexcept;

	private:
		void updateInbounds(std::vector<elem*>&& next);

		// void moveOwnerShip();
	};


}
