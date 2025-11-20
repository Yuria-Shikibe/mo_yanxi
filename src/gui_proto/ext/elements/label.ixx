export module mo_yanxi.gui.elem.label;

export import mo_yanxi.gui.infrastructure;
export import mo_yanxi.font;
export import mo_yanxi.font.typesetting;
import std;

namespace mo_yanxi::gui{

using instr_buffer = std::vector<std::byte, mr::aligned_heap_allocator<std::byte, 16>>;

export void record_glyph_draw_instructions(
	instr_buffer& buffer,
	const font::typesetting::glyph_layout& glyph_layout,
	graphic::color color_scl
	);

struct text_style{
	font::typesetting::layout_policy policy;
	align::pos align;
	float scale;
};
export
struct label;

export
struct sync_label_terminal : react_flow::terminal<std::string>{
	label* label_;

	[[nodiscard]] explicit sync_label_terminal(label& label)
	: label_(std::addressof(label)){
	}

protected:
	void on_update(const std::string& data) override;
};

struct label : elem{
protected:
	const font::typesetting::parser* parser{&font::typesetting::global_parser};
	font::typesetting::glyph_layout glyph_layout{};

	float scale{1.f};
	bool text_expired{false};
	bool fit_{};

	layout::expand_policy expand_policy_{};
	sync_label_terminal* notifier_{};
public:
	math::vec2 max_fit_scale_bound{math::vectors::constant2<float>::inf_positive_vec2};
	std::optional<graphic::color> text_color_scl{};
	align::pos text_entire_align{align::pos::top_left};

	using elem::elem;

	void set_text(std::string_view text){
		if(glyph_layout.get_text() != text){
			glyph_layout.set_text(std::string{text});
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_text(const char* text){
		set_text(std::string_view{text});
	}

	void set_text(std::string&& text){
		if(glyph_layout.get_text() != text){
			glyph_layout.set_text(std::move(text));
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_typesetting_policy(font::typesetting::layout_policy policy){
		if(glyph_layout.set_policy(policy)){
			if(policy != font::typesetting::layout_policy::reserve) fit_ = false;
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	[[nodiscard]] layout::expand_policy get_expand_policy() const{
		return expand_policy_;
	}

	void set_expand_policy(const layout::expand_policy expand_policy){
		if(util::try_modify(expand_policy_, expand_policy)){
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_fit(bool fit = true){
		if(util::try_modify(fit_, fit)){
			if(fit){
				set_typesetting_policy(font::typesetting::layout_policy::reserve);
				glyph_layout.set_clamp_size(math::vectors::constant2<float>::inf_positive_vec2);
			}else{
				set_typesetting_policy(font::typesetting::layout_policy::auto_feed_line);
			}

			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void set_scale(float scale){
		if(util::try_modify(this->scale, scale)){
			text_expired = true;
			notify_isolated_layout_changed();
		}
	}

	void layout_elem() override{
		elem::layout_elem();

		if(text_expired){
			auto maxSz = restriction_extent.potential_extent();
			const auto resutlSz = layout_text(maxSz.fdim(boarder_extent()));
			if(!resutlSz.equals(content_extent())){
				notify_layout_changed(propagate_mask::force_upper);
			}
		}
	}

	[[nodiscard]] std::string_view get_text() const noexcept{
		return glyph_layout.get_text();
	}


	sync_label_terminal& request_react_node(){
		if(notifier_){
			return *notifier_;
		}
		auto& node = get_scene().request_react_node<sync_label_terminal>(*this);
		this->notifier_ = &node;
		return node;
	}

protected:
	void draw_content(const rect clipSpace) const override;

	bool resize_impl(const math::vec2 size) override{
		if(elem::resize_impl(size)){
			layout_text(content_extent());
			return true;
		}
		return false;
	}

	std::optional<math::vec2> pre_acquire_size_impl(layout::optional_mastering_extent extent) override{
		if(fit_){
			if(expand_policy_ == layout::expand_policy::passive){
				return std::nullopt;
			}

			auto ext = extent.potential_extent().inf_to0();

			if(expand_policy_ == layout::expand_policy::prefer){
				if(auto pref = get_prefer_content_extent())ext.max(*pref);
			}

			return ext;
		}

		if(expand_policy_ == layout::expand_policy::passive){
			return std::nullopt;
		}

		auto ext = layout_text(extent.potential_extent());
		extent.apply(ext);

		ext = extent.potential_extent().inf_to0();

		if(expand_policy_ == layout::expand_policy::prefer){
			if(auto pref = get_prefer_content_extent())ext.max(*pref);
		}

		return ext;
	}

	bool set_text_quiet(std::string_view text){
		if(glyph_layout.get_text() != text){
			glyph_layout.set_text(std::string{text});
			return true;
		}
		return false;
	}

	[[nodiscard]] math::vec2 get_glyph_abs_src() const noexcept{
		if(fit_){
			const auto sz = content_extent().min(max_fit_scale_bound);
			const auto rst = align::get_offset_of(text_entire_align, sz, content_bound_abs());

			return rst;
		}else{
			const auto sz = glyph_layout.extent();
			const auto rst = align::get_offset_of(text_entire_align, sz, content_bound_abs());

			return rst;
		}

	}

	math::vec2 layout_text(math::vec2 bound){
		if(bound.area() < 32) return {};
		if(fit_){
			if(text_expired){
				glyph_layout.clear();
				parser->operator()(glyph_layout, 1);
				text_expired = false;
			}

			return bound;
		} else{
			//TODO loose the relayout requirement
			if(text_expired){
				glyph_layout.set_clamp_size(bound);
				glyph_layout.clear();
				parser->operator()(glyph_layout, scale);
				text_expired = false;
			} else{
				if(glyph_layout.get_clamp_size() != bound){
					glyph_layout.set_clamp_size(bound);
					if(!glyph_layout.extent().equals(bound)){
						glyph_layout.clear();
					}
					if(glyph_layout.empty()){
						parser->operator()(glyph_layout, scale);
					}
				}
			}
		}

		return glyph_layout.extent();
	}

	[[nodiscard]] std::optional<font::typesetting::layout_pos_t> get_layout_pos(math::vec2 globalPos) const;

	void draw_text() const;
};

export struct async_label;

export
struct async_label_terminal : react_flow::terminal<const font::typesetting::glyph_layout*>{
	async_label* label;

	[[nodiscard]] explicit async_label_terminal(async_label& label)
	: label(std::addressof(label)){
	}

	void on_update(const font::typesetting::glyph_layout* const& data) override;
};

struct async_label_layout_config{
	math::vec2 bound{math::vectors::constant2<float>::inf_positive_vec2};
	float throughout_scale{1.f};
	font::typesetting::layout_policy layout_policy{font::typesetting::layout_policy::reserve};
};

struct async_label : elem{
	friend async_label_terminal;
private:
	using config_prov_node = react_flow::provider_cached<async_label_layout_config>;
	bool scale_text_to_fit{true};
	async_label_terminal* terminal{};
	config_prov_node* config_prov_{};

	layout::expand_policy expand_policy_{};
	instr_buffer draw_instr_buffer_{get_heap_allocator<std::byte>()};
	std::optional<graphic::color> text_color_scl{};

public:
	align::pos text_entire_align{align::pos::top_left};

	[[nodiscard]] async_label(scene& scene, elem* parent)
	: elem(scene, parent), terminal{&get_scene().request_react_node<async_label_terminal>(*this)}{
	}

	~async_label() override{
		if(terminal)get_scene().erase_independent_react_node(*terminal, true);
	}

	void set_fit(bool is_scale_text_to_fit){
		if(util::try_modify(scale_text_to_fit, is_scale_text_to_fit)){
			notify_layout_changed(propagate_mask::force_upper);
		}
	}

protected:
	void draw_content(const rect clipSpace) const override;

	void update_draw_buffer(const font::typesetting::glyph_layout& layout){
		auto color = text_color_scl.value_or(graphic::colors::white);
		color.mul_a(get_draw_opacity());
		record_glyph_draw_instructions(draw_instr_buffer_, layout, color);
	}

public:
	config_prov_node* set_as_config_prov(){
		if(!config_prov_){
			config_prov_ = &get_scene().request_react_node<config_prov_node>(*this);
			config_prov_->update_value({
				.layout_policy = font::typesetting::layout_policy::def
			});
		}

		return config_prov_;
	}

	void set_dependency(react_flow::node& glyph_layout_prov, bool link_to_prov_if_any = true){
		if(terminal->get_inputs()[0] != std::addressof(glyph_layout_prov)){
			terminal->disconnect_self_from_context();
			terminal->connect_predecessor(glyph_layout_prov);
			notify_layout_changed(propagate_mask::local | propagate_mask::force_upper);

			if(link_to_prov_if_any && config_prov_){
				config_prov_->connect_successors(glyph_layout_prov);
			}
		}
	}

	[[nodiscard]] layout::expand_policy get_expand_policy() const noexcept{
		return expand_policy_;
	}

	void set_expand_policy(const layout::expand_policy expand_policy){
		if(util::try_modify(expand_policy_, expand_policy)){
			notify_layout_changed(propagate_mask::local | propagate_mask::force_upper);
		}
	}

	[[nodiscard]] std::optional<graphic::color> get_text_color_scl() const{
		return text_color_scl;
	}

	void set_text_color_scl(const std::optional<graphic::color> text_color_scl){
		if(util::try_modify(this->text_color_scl, text_color_scl)){
			if(auto glyph = terminal->request(false)){
				update_draw_buffer(**glyph);
			}
		}
	}

protected:
	void on_opacity_changed(float previous) override{
		if(auto glyph = terminal->request(false)){
			update_draw_buffer(**glyph);
		}
	}

	std::optional<math::vec2> pre_acquire_size_impl(layout::optional_mastering_extent extent) override{
		auto rst = [&, this] -> std::optional<math::vec2>{
			if(expand_policy_ == layout::expand_policy::passive){
				return std::nullopt;
			}

			if(!terminal){
				if(expand_policy_ == layout::expand_policy::prefer){
					return get_prefer_content_extent();
				}
				return std::nullopt;
			}

			if(const auto playout = terminal->request(true)){
				auto ext = extent.potential_extent().inf_to0();
				ext.max((*playout)->extent());

				if(expand_policy_ == layout::expand_policy::prefer){
					if(const auto pref = get_prefer_content_extent()){
						ext.max(*pref);
					}
				}

				return ext;
			}

			if(expand_policy_ == layout::expand_policy::prefer){
				auto ext = extent.potential_extent().inf_to0();
				if(const auto pref = get_prefer_content_extent()){
					ext.max(*pref);
				}
				return ext;
			}

			return std::nullopt;
		}();

		if(config_prov_){
			config_prov_->update_value<true>(&async_label_layout_config::bound, extent.potential_extent());
		}

		return rst;
	}
};

export
struct label_layout_node : react_flow::modifier_transient<const font::typesetting::glyph_layout*, async_label_layout_config, std::string>{
private:
	const font::typesetting::parser* parser_{&font::typesetting::global_parser};

	float last_scale_{1.f};
	std::atomic_uint current_idx_{0};
	font::typesetting::glyph_layout layout_[2]{};

public:
	[[nodiscard]] label_layout_node()
	: modifier_transient(react_flow::async_type::async_latest){
	}

protected:
	react_flow::request_pass_handle<const font::typesetting::glyph_layout*> request_raw(bool allow_expired) override{
		if(get_dispatched() > 0 && !allow_expired){
			return std::unexpected{react_flow::data_state::expired};
		}
		return react_flow::request_pass_handle<const font::typesetting::glyph_layout*>{layout_ + current_idx_.load(std::memory_order::acquire)};
	}


	std::optional<const font::typesetting::glyph_layout*> operator()(
		const std::stop_token& stop_token,
		async_label_layout_config&& param,
		std::string&& str
	) override{
		const unsigned idx = !static_cast<bool>(current_idx_.load(std::memory_order::acquire));
		auto& glayout = layout_[idx];

		if(glayout.get_text() != str){
			glayout.set_text(std::move(str));
			glayout.clear();
		}

		if(glayout.get_clamp_size() != param.bound){
			glayout.set_clamp_size(param.bound);
			glayout.clear();
		}

		if(glayout.set_policy(param.layout_policy)){
			glayout.clear();
		}

		if(util::try_modify(last_scale_, param.throughout_scale)){
			glayout.clear();
		}


		if(stop_token.stop_requested())return std::nullopt;

		if(glayout.empty()){
			parser_->operator()(glayout, param.throughout_scale);
			current_idx_.store(idx, std::memory_order::release);
		}

		return std::addressof(glayout);
	}

	std::optional<const font::typesetting::glyph_layout*> operator()(
		const std::stop_token& stop_token, const async_label_layout_config& policy, const std::string& str) override{

		//TODO replace with decay copy (auto{})
		return this->operator()(stop_token, async_label_layout_config(policy), std::string{str});
	}
};
}
