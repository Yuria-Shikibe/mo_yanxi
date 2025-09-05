//
// Created by Matrix on 2025/4/27.
//

export module mo_yanxi.ui.creation.file_selector;

export import mo_yanxi.ui.elem.table;
export import mo_yanxi.ui.primitives;
import mo_yanxi.ui.elem.list;

import mo_yanxi.ui.elem.scroll_pane;
import mo_yanxi.ui.elem.image_frame;

import mo_yanxi.core.platform;

import mo_yanxi.ui.elem.label;
import mo_yanxi.ui.elem.text_input_area;
import mo_yanxi.ui.elem.button;

import mo_yanxi.ui.assets;


import mo_yanxi.history_stack;
import std;


namespace mo_yanxi::ui::creation{
	constexpr std::string_view extension_of(const std::string_view path) noexcept{
		const auto pos = path.rfind('.');
		if(pos == std::string_view::npos)return {};
		return path.substr(pos);
	}

	export enum struct file_sort_mode{
		ascend = 0,
		descend = 1,
		name = 0b0010,
		time = 0b0100,
		size = 0b1000,
	};

	export constexpr file_sort_mode operator|(const file_sort_mode l, const file_sort_mode r) noexcept{
		return file_sort_mode{(std::to_underlying(l) | std::to_underlying(r))};
	}

	export constexpr file_sort_mode operator-(const file_sort_mode l, const file_sort_mode r) noexcept{
		return file_sort_mode{(std::to_underlying(l) & ~std::to_underlying(r))};
	}

	export constexpr bool operator&(const file_sort_mode l, const file_sort_mode r) noexcept{
		return static_cast<bool>(std::to_underlying(l) & std::to_underlying(r));
	}

	struct file_path_sorter{
	private:
		file_sort_mode type{};
		using target_type = std::filesystem::path;

		template <bool flip>
		struct file_three_way_comparator{
			static auto operator()(const target_type& lhs, const target_type& rhs) noexcept{
				if(std::filesystem::is_directory(lhs)){
					if(std::filesystem::is_directory(rhs)){
						return lhs <=> rhs;
					}

					if constexpr(flip){
						return std::strong_ordering::greater;
					}else{
						return std::strong_ordering::less;
					}
				}

				if(std::filesystem::is_directory(rhs)){
					if constexpr(flip){
						return std::strong_ordering::less;
					}else{
						return std::strong_ordering::greater;
					}
				}

				return lhs <=> rhs;
			}
		};
	public:
		[[nodiscard]] constexpr file_path_sorter() = default;

		[[nodiscard]] constexpr explicit(false) file_path_sorter(const file_sort_mode type)
			: type(type){
		}

		[[nodiscard]] constexpr bool operator()(const target_type& lhs, const target_type& rhs) const noexcept{
			using namespace std::filesystem;
			switch(std::to_underlying(type)){
			case std::to_underlying(file_sort_mode::name | file_sort_mode::descend) :{
				return std::is_gt(file_three_way_comparator<true>{}(lhs, rhs));
			}
			case std::to_underlying(file_sort_mode::name | file_sort_mode::ascend):{
				return std::is_lt(file_three_way_comparator<false>{}(lhs, rhs));
			}
			case std::to_underlying(file_sort_mode::time | file_sort_mode::descend):{
				return std::ranges::greater{}(last_write_time(lhs), last_write_time(rhs));
			}
			case std::to_underlying(file_sort_mode::time | file_sort_mode::ascend):{
				return std::ranges::less{}(last_write_time(lhs), last_write_time(rhs));
			}
			case std::to_underlying(file_sort_mode::size | file_sort_mode::descend):{
				return std::ranges::greater{}(file_size(lhs), file_size(rhs));
			}
			case std::to_underlying(file_sort_mode::size | file_sort_mode::ascend):{
				return std::ranges::less{}(file_size(lhs), file_size(rhs));
			}
			default : return std::is_gt(file_three_way_comparator<false>{}(lhs, rhs));
			}
		}
	};


	export
	class file_selector : public table{
	protected:
		using path = std::filesystem::path;



		struct file_entry : table{
			file_selector& get_file_selector() const noexcept{
				//TODO make it static cast
				//-> table -> scroll pane -> right table -> selector
				return dynamic_cast<file_selector&>(*elem::get_parent()->get_parent()->get_parent()->get_parent());
			}

			path path{};

			explicit file_entry(scene* scene, group* group, file_selector::path&& entry_path) :
				table(scene, group), path(std::move(entry_path)){
				interactivity = interactivity::intercepted;
				property.fill_parent.x = true;

				bool is_dir = std::filesystem::is_directory(path);
				bool is_root = path == path.parent_path();

				// std::println("{}?", path.parent_path().string());

				if(is_root){

				}else if(is_dir){
					set_style(theme::styles::whisper);
				}else{
					set_style(theme::styles::humble);
				}

				template_cell.set_pad(align::spacing{}.set(8));

				{
					auto hdl = this->emplace<icon_frame>(is_dir ? theme::icons::folder_general : theme::icons::file_general);
					hdl->prop().set_empty_drawer();
					hdl->prop().boarder.set(4);
					hdl.cell().set_size(60);
					hdl.cell().set_pad({.right = 4});
				}

				{
					auto hdl = this->emplace<label>();
					hdl->prop().set_empty_drawer();
					hdl->prop().boarder.set(4);
					hdl->text_entire_align = align::pos::center_left;
					hdl->set_scale(.55f);
					hdl->set_policy(font::typesetting::layout_policy::auto_feed_line);
					auto str = std::format("{:?}", (is_root ?  path.string() : path.filename().string()));
					std::string_view sv = str;
					sv.remove_prefix(1);
					sv.remove_suffix(1);
					hdl->set_text(sv);

					hdl.cell().set_external({false, true});
				}

				set_edge_pad(0);
			}

			void update(float delta_in_ticks) override{
				elem::update(delta_in_ticks);
				activated = get_file_selector().has_selected(path);
			}

			input_event::click_result on_click(const input_event::click click_event) override{
				if(util::is_valid_release_click(*this, click_event)){
					auto& menu = get_file_selector();
					if(std::filesystem::is_directory(path)){
						menu.visit_directory(path);
					}else{
						if(menu.main_selection == path){
							menu.main_selection.reset();
						}else{
							menu.main_selection = this->path;
						}
					}
				}

				return elem::on_click(click_event);
			}
		};

		table* menu{};
		list* entries{};
		label* current_entry_bar{};

		path current{};
		history_stack<path> history{};

		std::string search_key{};
		std::unordered_set<path> cared_suffix_{};
		std::optional<path> main_selection{};
		file_sort_mode sortType{file_sort_mode::name | file_sort_mode::ascend};

		[[nodiscard]] bool is_suffix_met(const path& path) const{
			return cared_suffix_.empty() || cared_suffix_.contains(path.extension());
		}

		[[nodiscard]] bool cared_file(const path& path) const noexcept{
			return true
				 && (search_key.empty() || path.filename().string().contains(search_key))
				 && (std::filesystem::is_directory(path) || is_suffix_met(path));
		}

		bool try_add_visit_history(path&& where) noexcept{
			if(const auto p = history.try_get(); p && *p == where)return false;
			history.push(std::move(where));
			return true;
		}

		bool try_add_visit_history(const path& where){
			return try_add_visit_history(path{where});
		}

		void goto_dir_unchecked(const path& path){
			if(current != path)set_current_path(path);
		}


		void popVisitedAndResume(){
			history.to_prev();
			history.truncate();
			if(const auto back = history.pop_and_get()){
				visit_directory(std::move(*back));
			}else{
				visit_root_directory();
			}
		}


		void transformListToEntryElements(std::vector<std::filesystem::path> unsorted_paths){
			std::ranges::sort(unsorted_paths, file_path_sorter{sortType});

			entries->clear_children();

			for (auto&& path : unsorted_paths){
				auto hdl = entries->emplace<file_entry>(std::move(path));

				hdl.cell().set_external().set_pad({4, 4});
			}
		}

		void build_ui() noexcept{
			current_entry_bar->set_text(std::format("..< | {:?}", current.string()));

			if(!std::filesystem::exists(current) || !std::filesystem::is_directory(current)){
				auto pathes = core::plat::get_drive_letters();
				transformListToEntryElements(
					pathes
					| std::views::as_rvalue
					| std::views::transform(convert_to<std::filesystem::path>{})
					| std::views::filter(std::bind_front(&file_selector::cared_file, this))
					| std::ranges::to<std::vector>());
			}else{
				try{
					transformListToEntryElements(std::filesystem::directory_iterator(current)
						| std::views::transform(&std::filesystem::directory_entry::path)
						| std::views::filter(std::bind_front(&file_selector::cared_file, this))
						| std::ranges::to<std::vector>());
				}catch(...){
					popVisitedAndResume();
				}
			}
		}

		void set_current_path(path&& current_path) noexcept{
			current = std::move(current_path);
			build_ui();
		}

		void set_current_path(const path& current_path) noexcept{
			set_current_path(path{current_path});
		}

	public:
		[[nodiscard]] file_selector(scene* scene, group* group)
			: table(scene, group){

			interactivity = interactivity::children_only;

			const auto m = this->emplace<table>();
			m->template_cell.set_height_from_scale().set_pad({.bottom = 8});
			m.cell().set_width(80.f).set_pad({.right = 8});
			menu = &m.elem();

			{
				auto b = m->end_line().emplace<button<icon_frame>>(theme::icons::left);
				b->set_style(theme::styles::no_edge);
				b->set_button_callback(button_tags::general, [this]{
					undo();
				});
				b->checkers.setDisableProv([this]{
					return !history.has_prev();
				});
			}

			{
				auto b = m->end_line().emplace<button<icon_frame>>(theme::icons::right);
				b->set_style(theme::styles::no_edge);
				b->set_button_callback(button_tags::general, [this]{
					redo();
				});
				b->checkers.setDisableProv([this]{
					return !history.has_next();
				});
			}



			auto rtable = this->emplace<table>();
			rtable->set_style();

			auto return_to_parent_button = rtable->emplace<button<label>>();
			return_to_parent_button->set_scale(.65f);
			return_to_parent_button->prop().boarder.set_vert(12);
			return_to_parent_button->set_text(std::format("..< | {:?}", current.string()));
			return_to_parent_button->set_button_callback(button_tags::general, [this]{
				visit_parent_directory();
			});
			return_to_parent_button->checkers.setDisableProv([this]{
				return !std::filesystem::exists(current);
			});
			return_to_parent_button.cell().set_external({false, true}).set_pad({.bottom = 8});

			current_entry_bar = std::to_address(return_to_parent_button);

			auto pane = rtable->end_line().emplace<scroll_pane>();
			pane->prop().set_empty_drawer();
			entries = &pane->function_init([](list& table){
				table.set_style();
			});

			visit_directory(std::filesystem::current_path());
		}

		void clear_history(){
			history.clear();
		}

		void undo(){
			history.to_prev();
			if(const auto cur = history.try_get()){
				goto_dir_unchecked(*cur);
			}
		}

		void redo(){
			history.to_next();

			if(const auto cur = history.try_get()){
				goto_dir_unchecked(*cur);
			}
		}

		void set_cared_suffix(const std::initializer_list<std::string_view> suffix){
			cared_suffix_.clear();
			for (const auto & basic_string_view : suffix){
				cared_suffix_.insert(basic_string_view);
			}

			build_ui();
		}

		[[nodiscard]] bool has_selected(const path& path) const noexcept{
			if(main_selection == path){
				return true;
			}

			return false;
		}

		[[nodiscard]] bool is_under_root_dir() const noexcept{
			return history.get().empty();
		}

		void visit_parent_directory(){
			if(auto& currentPath = history.get(); currentPath.has_parent_path()){
				if(currentPath.parent_path() != currentPath){
					visit_directory(currentPath.parent_path());
				}
				else visit_root_directory();
			}else{
				visit_root_directory();
			}
		}

		void visit_root_directory() noexcept{
			try_add_visit_history({});
			goto_dir_unchecked({});
		}

		void visit_directory(path&& path){
			if(!std::filesystem::exists(path) || !std::filesystem::is_directory(path)){
				visit_root_directory();
				return;
			}

			if(!try_add_visit_history(path)){
				return;
			}

			set_current_path(std::move(path));
		}

		void visit_directory(const path& where){
			visit_directory(path{where});
		}

		[[nodiscard]] const std::optional<path>& get_current_main_select() const noexcept{
			return main_selection;
		}

		file_selector(const file_selector& other) = delete;
		file_selector(file_selector&& other) noexcept = delete;
		file_selector& operator=(const file_selector& other) = delete;
		file_selector& operator=(file_selector&& other) noexcept = delete;

		//INTERFACE:

		virtual bool check_path() const{
			return true;
		}

		virtual void yield_path(){

		}

	protected:
		[[nodiscard]] bool is_suffix_met_at_create(const path& file_name) const{
			return (!file_name.has_extension() && cared_suffix_.size() == 1) || is_suffix_met(file_name);
		}

		[[nodiscard]] bool is_file_preferred(const path& file_name) const{
			if(!is_suffix_met_at_create(file_name))return false;
			if(file_name.has_parent_path())return false;
			if(std::filesystem::exists(current / file_name))return false;

			return true;
		}

		bool create_file(const path& file_name){
			auto p = current / file_name;
			if(!p.has_extension() && cared_suffix_.size() == 1){
				p.replace_extension(*cared_suffix_.begin());
			}

			if(const std::ofstream stream{p, std::ios::app}; stream.is_open()){
				set_current_path(current);
				build_ui();
				return true;
			}
			return false;
		}

		void add_file_create_button(){
			auto b = menu->end_line().emplace<button<icon_frame>>(theme::icons::plus);
			b->set_tooltip_state(tooltip_create_info{
				.layout_info = {
					.follow = tooltip_follow::owner,
					.align_owner = align::pos::center_right,
					.align_tooltip = align::pos::center_left,
				},
				.use_stagnate_time = false,
				.auto_release = false,
				.min_hover_time = tooltip_create_info::disable_auto_tooltip
			}, [this](ui::table& table){

				auto cb = table.emplace<button<icon_frame>>(theme::icons::check);
				cb->set_style(theme::styles::no_edge);
				cb.cell().set_width(60);
				cb.cell().pad.right = 8;


				auto area = table.emplace<text_input_area>();
				area->set_scale(.75f);
				area->set_style(theme::styles::no_edge);

				area->add_file_banned_characters();
				area.cell().set_external();

				cb->set_button_callback(button_tags::general, [this, &t = area.elem()]{
					create_file(t.get_text());
				});

				cb->checkers.setDisableProv([this, &t = area.elem()](){
					const auto txt = t.get_text();
					if(txt.empty())return true;
					return !is_file_preferred(txt);
				});

			});

			b->set_style(theme::styles::no_edge);
			b->set_button_callback(button_tags::general, [](elem& elem){
				if(!elem.has_tooltip()){
					elem.build_tooltip();
				}else{
					elem.tooltip_notify_drop();
				}
			});
		}
	};

	export
	template <std::derived_from<elem> T, std::predicate<const file_selector&, const T&> Checker, std::predicate<file_selector&, T&> Yielder>
	struct file_selector_create_info{
		T& requester;
		Checker checker;
		Yielder yielder;
		bool add_file_create_button{};
	};

	export
	template <std::derived_from<elem> T, std::predicate<const file_selector&, const T&> Checker, std::predicate<file_selector&, T&> Yielder>
	file_selector& create_file_selector(file_selector_create_info<T, Checker, Yielder>&& create_info){
		struct selector : file_selector{
			T* owner;
			std::decay_t<Checker> checker;
			std::decay_t<Yielder> yielder;

			[[nodiscard]] selector(scene* scene, group* group, file_selector_create_info<T, Checker, Yielder>& create_info)
				: file_selector(scene, group), owner(&create_info.requester), checker(std::forward<Checker>(create_info.checker)), yielder(std::forward<Yielder>(create_info.yielder)){

				auto close_b = menu->end_line().emplace<button<icon_frame>>(theme::icons::close);
				close_b->set_style(theme::styles::no_edge);
				close_b->set_button_callback(button_tags::general, [this]{
					dialog_notify_drop();
				});

				auto confirm_b = menu->end_line().emplace<button<icon_frame>>(theme::icons::check);
				confirm_b->set_style(theme::styles::no_edge);
				confirm_b->set_button_callback(button_tags::general, [this]{
					yield_path();
				});
				confirm_b->checkers.setDisableProv([this]{
					return !check_path();
				});

				if(create_info.add_file_create_button){
					add_file_create_button();
				}

			}

			void yield_path() override{
				if(std::invoke(yielder, *this, *owner)){
					dialog_notify_drop();
				}
			}

			bool check_path() const override{
				return std::invoke(checker, *this, *owner);
			}

			// esc_flag on_esc() override{
			// 	dialog_notify_drop();
			// 	return esc_flag::intercept;
			// }
		};


		scene& scene = *create_info.requester.get_scene();

		file_selector& selector = scene.dialog_manager.emplace<struct selector>({}, create_info);
		selector.prop().fill_parent = {true, true};
		return selector;
	}
}
