//
// Created by Matrix on 2025/5/9.
//

export module mo_yanxi.game.world.hud;

import mo_yanxi.ui.basic;
import mo_yanxi.ui.root;
import mo_yanxi.ui.manual_table;
import mo_yanxi.ui.table;

import mo_yanxi.game.ecs.component.manage;
import mo_yanxi.game.path_sequence;
import mo_yanxi.game.ecs.system.collision;
import mo_yanxi.game.world.graphic;

import std;

namespace mo_yanxi::game::world{
	export
	struct hud {
		struct hud_context{
			ecs::component_manager* component_manager{};
			ecs::system::collision_system* collision_system{};
			world::graphic_context* graphic_context{};
		};

		struct path_editor{
		private:
			ecs::entity_ref target{};

			path_seq current_path{};

			float confirm_animation_progress_{};
			static constexpr float confirm_animation_progress_duration = 120;

		public:
			void reset() noexcept{
				target = nullptr;
				current_path = {};
			}

			void active(const ecs::entity_ref& target) noexcept;

			explicit operator bool() const noexcept{
				return static_cast<bool>(target);
			}

			void apply() noexcept;

			void on_key_input(core::ctrl::key_pack key_pack) noexcept;

			void on_click(ui::input_event::click click);

			void draw(const ui::elem& elem, graphic::renderer_ui& renderer) const;

			void update(float delta_in_tick){
				if(confirm_animation_progress_ >= 0){
					if(confirm_animation_progress_ > confirm_animation_progress_duration){
						confirm_animation_progress_ = -1;
					}else{
						confirm_animation_progress_ += delta_in_tick;
					}
				}

			}
		};

		struct hud_viewport : ui::manual_table{
			hud* hud{};
			hud_context context{};

		private:
			ecs::entity_ref main_selected{};
			std::unordered_set<ecs::entity_ref> selected{};

			ui::table* side_bar{};
			ui::table* button_bar{};

			path_editor path_editor_{};

			void build_hud();

			void build_entity_info();
		public:
			[[nodiscard]] hud_viewport(ui::scene* scene, ui::group* group)
				: manual_table(scene, group){
				// skip_inbound_capture = true;
				interactivity = ui::interactivity::enabled;
				build_hud();
			}

			void draw_content(const ui::rect clipSpace) const override;

			ui::input_event::click_result on_click(const ui::input_event::click click_event) override;

			void on_key_input(const core::ctrl::key_code_t key, const core::ctrl::key_code_t action, const core::ctrl::key_code_t mode) override;

			void update(float delta_in_ticks) override{
				manual_table::update(delta_in_ticks);
				if(!main_selected){
					main_selected = nullptr;
					path_editor_.reset();
				}

				if(path_editor_){
					path_editor_.update(delta_in_ticks);
				}
			}

		private:
			[[nodiscard]] math::vec2 get_transed(math::vec2 screen_pos) const noexcept;
		};
	private:
		ui::scene* scene;
		hud_viewport* viewport;

	public:
		void bind_context(const hud_context& ctx){
			viewport->context = ctx;
		}

		[[nodiscard]] hud();

		~hud();

		void focus_hud();

		hud(const hud& other) = delete;
		hud(hud&& other) noexcept = delete;
		hud& operator=(const hud& other) = delete;
		hud& operator=(hud&& other) noexcept = delete;
	};
}
