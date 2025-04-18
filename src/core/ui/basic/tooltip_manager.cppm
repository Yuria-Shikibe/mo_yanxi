module;

#include <cassert>

export module mo_yanxi.ui.basic:tooltip_manager;

export import mo_yanxi.ui.flags;

import :tooltip_interface;
import std;

namespace mo_yanxi::ui{
	export class tooltip_manager;

	struct ToolTipDrawInfo{
		elem* element{};
		bool belowScene{};

		friend bool operator==(const ToolTipDrawInfo& lhs, const ToolTipDrawInfo& rhs) noexcept{
			return lhs.element == rhs.element;
		}

		friend bool operator==(const ToolTipDrawInfo& lhs, const elem* rhs) noexcept{
			return lhs.element == rhs;
		}

		friend bool operator==(const elem* lhs, const ToolTipDrawInfo& rhs) noexcept{
			return lhs == rhs.element;
		}
	};

	struct tooltip_instance{
		friend tooltip_manager;
		elem_ptr element{};
		tooltip_owner* owner{};
		// bool below_scene{};

		math::vec2 last_pos{math::vectors::constant2<float>::SNaN};

	private:
		void update_layout(const tooltip_manager& manager);

		[[nodiscard]] bool isPosSet() const noexcept{
			return !last_pos.is_NaN();
		}
	};


	export class tooltip_manager{
	public:
		static constexpr float RemoveFadeTime = 15.f;
		static constexpr float MarginSize = 15.f;
	private:
		struct DroppedToolTip{
			elem_ptr element{};
			float remainTime{};
		};

	private:
		std::vector<ToolTipDrawInfo> drawSequence{};
		std::vector<DroppedToolTip> dropped{};

		//using deque to guarantee references are always valid
		std::deque<tooltip_instance> actives{};

		using ActivesItr = decltype(actives)::iterator;

	public:
		scene* scene{};

		[[nodiscard]] tooltip_manager() = default;

		// bool hasInstance(const TooltipOwner& owner){
		// 	return std::ranges::contains(actives | std::views::reverse | std::views::transform(&ValidTooltip::owner), &owner);
		// }

		[[nodiscard]] tooltip_owner* get_top_focus() const noexcept{
			return actives.empty() ? nullptr : actives.back().owner;
		}

		[[nodiscard]] math::vec2 get_cursor_pos() const noexcept;

		tooltip_instance& append_tooltip(
			tooltip_owner& owner,
			bool belowScene = false,
			bool fade_in = true);

		tooltip_instance* try_append_tooltip(tooltip_owner& owner,
			bool belowScene = false,
			bool fade_in = true){
			if(!owner.has_tooltip() && owner.tooltip_should_build(get_cursor_pos())){
				return &append_tooltip(owner, belowScene, fade_in);
			}
			return nullptr;
		}

		void update(float delta_in_time);

		bool requestDrop(const tooltip_owner* owner){
			const auto be = std::ranges::find(actives, owner, &tooltip_instance::owner);

			return dropFrom(be);
		}

		void draw_above() const;
		void draw_below() const;

		bool is_below_scene(const elem* element) const noexcept{
			for (const auto & draw : drawSequence){
				if(draw.element == element){
					return draw.belowScene;
				}
			}

			return false;
		}

		auto& get_active_tooltips() noexcept{
			return actives;
		}

		void clear() noexcept{
			drawSequence.clear();
			dropAll();
			dropped.clear();
		}

		esc_flag on_esc();

		[[nodiscard]] std::span<const ToolTipDrawInfo> get_draw_sequence() const{
			return drawSequence;
		}

	private:
		bool drop(ActivesItr be, ActivesItr se);

		bool dropOne(const ActivesItr where){
			return drop(where, std::next(where));
		}

		bool dropBack(){
			assert(!actives.empty());
			return drop(std::prev(actives.end()), actives.end());
		}

		bool dropAll(){
			return drop(actives.begin(), actives.end());
		}

		bool dropFrom(const ActivesItr where){
			return drop(where, actives.end());
		}

		void updateDropped(float delta_in_time);
	};
}
