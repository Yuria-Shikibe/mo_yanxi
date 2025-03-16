
export module mo_yanxi.ui.table;

export import mo_yanxi.ui.layout.policies;
export import mo_yanxi.ui.layout.cell;
export import mo_yanxi.ui.celled_group;
export import mo_yanxi.ui.util;
import std;

namespace mo_yanxi::ui{
	using table_size_t = unsigned;

	export struct table_cell_adaptor : cell_adaptor<mastering_cell>{
		using cell_adaptor::cell_adaptor;

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
		[[nodiscard]] constexpr std::array<float align::spacing::*, 4> get_pad_ptr() const noexcept{
			if(policy_ == layout_policy::vertical){
				return {
					&align::spacing::top,
					&align::spacing::bottom,

					&align::spacing::left,
					&align::spacing::right,
				};
			}else{
				return {
					&align::spacing::left,
					&align::spacing::right,

					&align::spacing::top,
					&align::spacing::bottom,
				};
			}
		}

		[[nodiscard]] constexpr std::array<stated_size stated_extent::*, 2> get_extent_ptr() const noexcept{
			if(policy_ == layout_policy::vertical){
				return {
					&stated_extent::height,
					&stated_extent::width,
				};
			}else{
				return {
					&stated_extent::width,
					&stated_extent::height
				};
			}
		}

		template <typename T = float>
		[[nodiscard]] constexpr auto get_vec_ptr() const noexcept{
			if(policy_ == layout_policy::vertical){
				return std::array{
					&math::vector2<T>::y,
					&math::vector2<T>::x,
				};
			}else{
				return std::array{
					&math::vector2<T>::x,
					&math::vector2<T>::y,
				};
			}
		}

		pre_layout_result layout_masters(
			const std::span<cell_adaptor_type> cells){
			auto view = cells | std::views::chunk_by([](const cell_adaptor_type& current, const cell_adaptor_type&){
				return !current.line_feed;
			}) | std::views::enumerate;

			const auto [
				pad_major_src,
				pad_major_dst,
				pad_minor_src,
				pad_minor_dst] = get_pad_ptr();

			const auto [extent_major, extent_minor] = get_extent_ptr();

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
				const auto [major_target, minor_target] = get_vec_ptr<>();

				cap.*major_target = std::ranges::fold_left(get_majors() | std::views::transform(&table_head::get_captured_size), 0.f, std::plus{});
				cap.*minor_target = std::ranges::fold_left(get_minors() | std::views::transform(&table_head::get_captured_size), 0.f, std::plus{});
			}
			{
				const auto [major_target, minor_target] = get_vec_ptr<table_size_t>();

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

			const auto [extent_major, extent_minor] = get_extent_ptr();
			const auto [major_target, minor_target] = get_vec_ptr<>();


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
			math::vec2 entire_offset = {}
			){
			auto view = cells | std::views::chunk_by([](const cell_adaptor_type& current, const cell_adaptor_type&){
				return !current.line_feed;
			}) | std::views::enumerate;

			const auto extent_major = &stated_extent::width;
			const auto extent_minor = &stated_extent::height;

			const auto major_target = &math::vec2::x;
			const auto minor_target = &math::vec2::y;

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


					stated_extent ext;
					ext.*extent_major = head_major.max_size;
					ext.*extent_minor = head_minor.max_size;

					//TODO adjust cell bound according to cell size
					elem.cell.allocated_region.src = current_position + src_off + entire_offset;
					elem.cell.allocated_region.set_size(size);
					elem.apply(parent, ext);

					line_stride = math::max(line_stride, src_off.y + elem.cell.allocated_region.height() + dst_off.y);
					current_position.x += src_off.x + elem.cell.allocated_region.width() + dst_off.x;
				}

				current_position.x = 0;
				current_position.y += line_stride;
				line_stride = 0;
			}
		}
	};

	export struct table : celled_group<table_cell_adaptor>{

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
	protected:
		math::vec2 pre_layout(table_layout_context& context, stated_extent constrain, bool size_to_constrain){
			auto size = context.allocate_cells(cells, constrain.potential_max_size());

			if(constrain.width.dependent()){
				size.x += property.boarder.width();
			}else{
				size.x = size_to_constrain ? constrain.width + property.boarder.width() : get_size().x;
			}

			if(constrain.height.dependent()){
				size.y += property.boarder.height();
			}else{
				size.y = size_to_constrain ? constrain.height + property.boarder.height() : get_size().y;
			}

			return size;
		}

		std::optional<math::vec2> pre_acquire_size(stated_extent extent) override{
			const auto grid = util::countRowAndColumn_toVector(cells, &table_cell_adaptor::line_feed);
			if(grid.empty()) return std::nullopt;

			// elem::resize_quiet(get_size().min(extent.potential_max_size()));
			table_layout_context context{layout_policy, std::ranges::max(grid), static_cast<table_size_t>(grid.size())};
			if(extent.width.mastering()){extent.width.value = math::clamp_positive(extent.width.value - property.boarder.width());}
			if(extent.height.mastering()){extent.height.value = math::clamp_positive(extent.height.value - property.boarder.height());}


			auto size = pre_layout(context, extent, true);

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
			elem::resize_quiet(size);

			auto off = align::get_offset_of(entire_align, size, rect{tags::from_extent, property.content_src_offset(), content_size()});

			context.resize_and_set_elems(cells, *this);
		}

		 // grid{};
		align::pos entire_align{align::pos::center};
		layout_policy layout_policy{layout_policy::horizontal};

	};
}
