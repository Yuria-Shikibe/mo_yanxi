
export module mo_yanxi.ui.table;

export import mo_yanxi.ui.layout.policies;
export import mo_yanxi.ui.layout.cell;
export import mo_yanxi.ui.celled_group;
export import mo_yanxi.ui.util;
import std;

namespace mo_yanxi::ui{
	using table_size_t = unsigned;

	export struct table_cell_adaptor : cell_adaptor<mastering_cell>{
		[[nodiscard]] constexpr table_cell_adaptor() noexcept = default;

		[[nodiscard]] constexpr table_cell_adaptor(elem* element, const mastering_cell& cell) noexcept
			: cell_adaptor{element, cell}{}

		bool line_feed{};
	};

	class layout_line{
	public:
		std::vector<stated_extent> size_data{};
	};

	class table_layout_context{
		using cell_adaptor_type = table_cell_adaptor;
		struct table_head{
			stated_size max_size{};
			float max_pad_src{};
			float max_pad_dst{};

			[[nodiscard]] constexpr float get_captured_size() const noexcept{
				return max_pad_src + max_pad_dst + max_size;
			}

			[[nodiscard]] constexpr bool mastering() const noexcept{
				return max_size.mastering();
			}
		};

		layout_policy policy_{};
		table_size_t max_major_size_{};
		std::vector<table_head> masters_{};

	public:
		[[nodiscard]] table_layout_context() = default;

		[[nodiscard]] table_layout_context(
			const layout_policy policy,
			const table_size_t max_major_size,
			const table_size_t max_minor_size
		) :
			policy_(policy),
			max_major_size_{max_major_size}, masters_(max_major_size + max_minor_size){
		}

		template <typename T>
		struct visit_result{
			T major;
			T minor;
		};

		struct pre_layout_result{
			math::vec2 captured_size{};
			math::vector2<table_size_t> masters{};
		};


		[[nodiscard]] constexpr table_size_t max_minor_size() const noexcept{
			return masters_.size() - max_major_size_;
		}

		[[nodiscard]] constexpr table_size_t max_major_size() const noexcept{
			return max_major_size_;
		}

		[[nodiscard]] constexpr auto& at_major(const table_size_t column) noexcept{
			return masters_[column];
		}

		[[nodiscard]] constexpr auto& at_minor(const table_size_t row) noexcept{
			return masters_[max_major_size_ + row];
		}

		[[nodiscard]] constexpr auto& at_major(const table_size_t column) const noexcept{
			return masters_[column];
		}

		[[nodiscard]] constexpr auto& at_minor(const table_size_t row) const noexcept{
			return masters_[max_major_size_ + row];
		}

		[[nodiscard]] constexpr auto& data() const noexcept{
			return masters_;
		}

		[[nodiscard]] std::span<const table_head> get_majors() const noexcept{
			return {masters_.data(), max_major_size_};
		}

		[[nodiscard]] std::span<const table_head> get_minors() const noexcept{
			return {masters_.data() + max_major_size_, max_minor_size()};
		}

		[[nodiscard]] std::span<table_head> get_majors() noexcept{
			return {masters_.data(), max_major_size_};
		}

		[[nodiscard]] std::span<table_head> get_minors() noexcept{
			return {masters_.data() + max_major_size_, max_minor_size()};
		}

		constexpr visit_result<table_head&> operator[](const table_size_t major, const table_size_t minor) noexcept{
			return {at_major(major), at_minor(minor)};
		}

		constexpr visit_result<const table_head&> operator[](const table_size_t major, const table_size_t minor) const noexcept{
			return {at_major(major), at_minor(minor)};
		}

	private:

		pre_layout_result layout_masters(
			const std::span<cell_adaptor_type> cells){
			auto view = cells | std::views::chunk_by([](const cell_adaptor_type& current, const cell_adaptor_type&){
				return !current.line_feed;
			}) | std::views::enumerate;

			const auto [
				pad_major_src,
				pad_major_dst,
				pad_minor_src,
				pad_minor_dst] = get_pad_ptr(policy_);

			const auto [extent_major, extent_minor] = get_extent_ptr(policy_);

			for (auto&& [idx_minor, line] : view){
				for (auto && [idx_major, elem] : line | std::views::enumerate){
					auto [head_major, head_minor] = (*this)[idx_major, idx_minor];

					head_major.max_pad_src = math::max(head_major.max_pad_src, elem.cell.pad.*pad_major_src);
					head_major.max_pad_dst = math::max(head_major.max_pad_dst, elem.cell.pad.*pad_major_dst);

					head_minor.max_pad_src = math::max(head_minor.max_pad_src, elem.cell.pad.*pad_minor_src);
					head_minor.max_pad_dst = math::max(head_minor.max_pad_dst, elem.cell.pad.*pad_minor_dst);

					const auto promoted = elem.cell.stated_extent.promote();
					head_major.max_size.try_promote_by((promoted.*extent_major).decay());
					if((promoted.*extent_minor).mastering())head_minor.max_size.promote(promoted.*extent_minor);
				}
			}

			math::vec2 cap{};
			math::vector2<table_size_t> masterings{};

			{
				const auto [major_target, minor_target] = get_vec_ptr<>(policy_);

				cap.*major_target = std::ranges::fold_left(get_majors() | std::views::transform(&table_head::get_captured_size), 0.f, std::plus{});
				cap.*minor_target = std::ranges::fold_left(get_minors() | std::views::transform(&table_head::get_captured_size), 0.f, std::plus{});
			}
			{
				const auto [major_target, minor_target] = get_vec_ptr<table_size_t>(policy_);

				masterings.*major_target = std::ranges::count_if(get_majors(), &table_head::mastering);
				masterings.*minor_target = std::ranges::count_if(get_minors(), &table_head::mastering);
			}

			return {cap, masterings};
		}

		/**
		 * @return elements captured size
		 */
		math::vec2 restricted_allocate_major(
			const std::span<const cell_adaptor_type> cells,
			math::vec2 valid_size,
			pre_layout_result pre_result){

			math::vec2 curSize = valid_size - pre_result.captured_size;

			auto view = cells | std::views::chunk_by([](const cell_adaptor_type& current, const cell_adaptor_type&){
				return !current.line_feed;
			}) | std::views::enumerate;

			const auto [extent_major, extent_minor] = get_extent_ptr(policy_);
			const auto [major_target, minor_target] = get_vec_ptr<>(policy_);


			//TODO when size in major is inf, pre acquire its size and try promote it to master,
			// if failed to get a pre_acquire size to promote the table head, discard it

			if(std::isinf(curSize.*major_target)){
				for(auto&& [idx_minor, line] : view){
					auto& head_minor = at_minor(idx_minor);

					for(auto&& [idx_major, elem] : line | std::views::enumerate){
						auto& head_major = at_major(idx_major);

						if((elem.cell.stated_extent.*extent_major).type == size_category::external){
							stated_extent ext;
							ext.*extent_major = {size_category::external};
							ext.*extent_minor = head_minor.max_size.mastering() ? head_minor.max_size : stated_size{size_category::external};

							if(auto size = elem.element->pre_acquire_size(ext)){
								head_major.max_size.promote(size.value().*major_target);
								head_minor.max_size.promote(
									math::min(size.value().*minor_target, curSize.*minor_target));
							}
						}
					}
				}

				curSize.*major_target = 0;
			}

			float total_weight{};
			for (auto& major : get_majors()){
				if(!major.max_size.mastering()){
					total_weight += major.max_size.value;
					major.max_size.value *= curSize.*major_target;
				}
			}


			float total_minor_weight{};
			for (auto&& [idx_minor, line] : view){
				auto& head_minor = at_minor(idx_minor);

				float line_external_consumes{};
				for (auto && [idx_major, elem] : line | std::views::enumerate){
					auto& head_major = at_major(idx_major);

					if(!head_major.mastering()){
						head_major.max_size.promote(head_major.max_size.value / total_weight);
					}

					if(!head_minor.mastering()){
						switch((elem.cell.stated_extent.*extent_minor).type){
						case size_category::scaling:{
							float valid = math::min(head_major.max_size * head_minor.max_size.value, curSize.*minor_target);
							head_minor.max_size.promote(valid);
							line_external_consumes = math::max(line_external_consumes, valid);
							break;
						}
						case size_category::external:{
							stated_extent ext;
							ext.*extent_major = head_major.max_size;
							ext.*extent_minor = {size_category::external};

							if(auto size = elem.element->pre_acquire_size(ext)){
								float valid = math::min(size.value().*minor_target, curSize.*minor_target);

								head_minor.max_size.promote(valid);
								line_external_consumes = math::max(line_external_consumes, valid);
							}
							break;
						}
						default: break;
						}
					}
				}

				curSize.*minor_target -= line_external_consumes;
				line_external_consumes = 0.;

				if(!head_minor.mastering()){
					total_minor_weight += head_minor.max_size.value;
				}
			}

			math::vec2 extent{};

			for (const auto & table_head : get_majors()){
				extent.*major_target += table_head.get_captured_size();
			}

			for (auto & table_head : get_minors()){
				if(!table_head.max_size.mastering()){
					table_head.max_size.promote(table_head.max_size.value / total_minor_weight * curSize.copy().inf_to0().*minor_target);
				}
				extent.*minor_target += table_head.get_captured_size();
			}

			return extent;
		}
	public:

		math::vec2 allocate_cells(
			const std::span<cell_adaptor_type> cells,
			math::vec2 valid_size){
			return restricted_allocate_major(cells, valid_size, layout_masters(cells));
		}

		void resize_and_set_elems(
			const std::span<cell_adaptor_type> cells,
			group& parent,
			math::frect region = {}
			){
			auto view = cells | std::views::chunk_by([](const cell_adaptor_type& current, const cell_adaptor_type&){
				return !current.line_feed;
			}) | std::views::enumerate;

			const auto [extent_major, extent_minor] = get_extent_ptr(policy_);
			const auto [major_target, minor_target] = get_vec_ptr<>(policy_);

			math::vec2 current_position{};
			for (auto&& [idx_minor, line] : view){
				float line_stride{};

				for (auto && [idx_major, elem] : line | std::views::enumerate){
					auto [head_major, head_minor] = (*this)[idx_major, idx_minor];
					math::vec2 src_off;
					src_off.*major_target = head_major.max_pad_src;
					src_off.*minor_target = head_minor.max_pad_src;

					math::vec2 size;
					size.*major_target = head_major.max_size;
					size.*minor_target = head_minor.max_size;

					math::vec2 dst_off;
					dst_off.*major_target = head_major.max_pad_dst;
					dst_off.*minor_target = head_minor.max_pad_dst;

					if(elem.cell.saturate && std::ranges::size(line) == 1){
						size.*major_target = region.size().*major_target - (src_off).*major_target - (dst_off).*major_target;
					}

					stated_extent ext;
					ext.*extent_major = head_major.max_size;
					ext.*extent_minor = head_minor.max_size;

					//TODO
					if((elem.cell.stated_extent.*extent_minor).dependent()){
						ext.*extent_minor = {size_category::external};
					}

					if((elem.cell.stated_extent.*extent_major).dependent() && !elem.cell.saturate){
						ext.*extent_major = {size_category::external};
					}


					//TODO adjust cell bound according to cell size
					elem.cell.allocated_region.src = current_position + src_off + region.get_src();
					elem.cell.allocated_region.set_size(size);
					elem.apply(parent, ext);

					const auto total_off = src_off + dst_off + elem.cell.allocated_region.size();

					line_stride = math::max(line_stride, total_off.*minor_target);
					current_position.*major_target += total_off.*major_target;
				}

				current_position.*major_target = 0;
				current_position.*minor_target += line_stride;
				line_stride = 0;
			}
		}
	};

	export struct table : universal_group<table_cell_adaptor::cell_type, table_cell_adaptor>{

		[[nodiscard]] table(scene* scene, group* group)
			: universal_group(scene, group, "table"){
		}

		void set_entire_align(const align::pos align){
			if(util::tryModify(entire_align, align)){
				notify_layout_changed(spread_direction::from_content);
			}
		}

		[[nodiscard]] align::pos get_entire_align() const{
			return entire_align;
		}

		void set_policy(layout_policy policy){
			if(policy != layout_policy){
				notify_layout_changed(spread_direction::from_content);
				layout_policy = policy;
			}
		}

		void layout() override{
			layout_directional();
			elem::layout();
		}

		table& end_line(){
			if(cells.empty())return *this;
			cells.back().line_feed = true;
			return *this;
		}

		void set_edge_pad(align::spacing pad){
			const auto grid = util::countRowAndColumn_toVector(cells, &table_cell_adaptor::line_feed);
			if(grid.empty()) return;
			auto end_idx = std::ranges::max(grid) - 1;

			auto view = cells | std::views::chunk_by([](const adaptor_type& current, const adaptor_type&){
				return !current.line_feed;
			}) | std::views::enumerate;

			bool changed{};

			for(auto&& [minor, line] : view){
				for(auto&& [major, elem] : line | std::views::enumerate){
					switch(layout_policy){
					case layout_policy::hori_major :{
						if(minor == 0){
							changed |= util::tryModify(elem.cell.pad.top, pad.top);
						}

						if(minor == grid.size() - 1){
							changed |= util::tryModify(elem.cell.pad.bottom, pad.bottom);
						}

						if(major == 0){
							changed |= util::tryModify(elem.cell.pad.left, pad.left);
						}

						if(major == end_idx || (elem.cell.saturate && std::ranges::size(line) == 1)){
							changed |= util::tryModify(elem.cell.pad.right, pad.right);
						}
						break;
					}
					case layout_policy::vert_major :{
						if(minor == 0){
							changed |= util::tryModify(elem.cell.pad.left, pad.left);
						}

						if(minor == grid.size() - 1){
							changed |= util::tryModify(elem.cell.pad.right, pad.right);
						}

						if(major == 0){
							changed |= util::tryModify(elem.cell.pad.top, pad.top);
						}

						if(major == end_idx || (elem.cell.saturate && std::ranges::size(line) == 1)){
							changed |= util::tryModify(elem.cell.pad.bottom, pad.bottom);
						}
					}
					default : break;
					}
				}
			}

			if(changed){
				notify_layout_changed(spread_direction::all_visible);
			}
		}

		void set_edge_pad(float pad){
			set_edge_pad({pad, pad, pad, pad});
		}

	public:
		std::optional<math::vec2> pre_acquire_size(stated_extent extent) override{
			const auto grid = util::countRowAndColumn_toVector(cells, &table_cell_adaptor::line_feed);
			if(grid.empty()) return std::nullopt;

			// elem::resize_quiet(get_size().min(extent.potential_max_size()));
			table_layout_context context{layout_policy, std::ranges::max(grid), static_cast<table_size_t>(grid.size())};

			auto size = pre_layout(context, clip_boarder_from(extent), true);

			return size;
		}


	protected:
		math::vec2 pre_layout(table_layout_context& context, stated_extent constrain, bool size_to_constrain){
			auto size = context.allocate_cells(cells, constrain.potential_max_size());

			const auto [extent_major, extent_minor] = get_extent_ptr(layout_policy);
			const auto [major_target, minor_target] = get_vec_ptr<>(layout_policy);
			const auto [
							pad_major_src,
							pad_major_dst,
							pad_minor_src,
							pad_minor_dst] = get_pad_ptr(layout_policy);

			auto pad_major = property.boarder.*pad_major_src + property.boarder.*pad_major_dst;
			auto pad_minor = property.boarder.*pad_minor_src + property.boarder.*pad_minor_dst;

			if((constrain.*extent_major).dependent()){
				size.*major_target += pad_major;
			}else{
				size.*major_target = size_to_constrain ? (constrain.*extent_major) + pad_major : get_size().*major_target;
			}

			if((constrain.*extent_minor).dependent()){
				size.*minor_target += pad_minor;
			}else{
				size.*minor_target = size_to_constrain ? (constrain.*extent_minor) + pad_minor : get_size().*minor_target;
			}

			return size;
		}

		void layout_directional(){
			const auto grid = util::countRowAndColumn_toVector(cells, &table_cell_adaptor::line_feed);
			if(grid.empty()) return;

			table_layout_context context{layout_policy, std::ranges::max(grid), static_cast<table_size_t>(grid.size())};

			auto extent = context_size_restriction;
			extent.collapse(content_size());

			auto size = pre_layout(context, extent, false);
			size.min(context_size_restriction.potential_max_size());
			elem::resize_masked(size);

			size -= property.boarder.get_size();
			size.max({});

			auto off = align::get_offset_of(entire_align, size, rect{tags::from_extent, {}, content_size()});

			context.resize_and_set_elems(cells, *this, rect{tags::from_extent, off, size});
		}

		 // grid{};
		align::pos entire_align{align::pos::center};
		layout_policy layout_policy{layout_policy::hori_major};

	};
}
