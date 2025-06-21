//
// Created by Matrix on 2025/6/2.
//

export module mo_yanxi.game.meta.chamber;

export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.game.ecs.component.hit_point;
export import mo_yanxi.game.ecs.component.drawer;
export import mo_yanxi.ui.table;

// import mo_yanxi.graphic.renderer.predecl;

import mo_yanxi.type_map;
import mo_yanxi.owner;
import mo_yanxi.game.srl;
import std;

#define NAME_OF(e) ((void)category::e, #e)

namespace mo_yanxi::game::meta::chamber{
	export struct chamber_instance_data;
	export
	struct ui_edit_context{
		chamber_instance_data* current_changed_building{};
		bool dynamic_energy_usage_changed{};

	};

	export
	struct ui_build_handle{
		struct promise_type;
		using handle = std::coroutine_handle<promise_type>;

		[[nodiscard]] ui_build_handle() noexcept = default;

		[[nodiscard]] explicit ui_build_handle(handle&& hdl)
			: hdl{std::move(hdl)}{}

		struct promise_type{
			bool has_ui{};

			[[nodiscard]] promise_type() = default;

			ui_build_handle get_return_object() noexcept{
				return ui_build_handle{handle::from_promise(*this)};
			}

			[[nodiscard]] static auto initial_suspend() noexcept{ return std::suspend_never{}; }

			[[nodiscard]] static auto final_suspend() noexcept{ return std::suspend_always{}; }

			std::suspend_always yield_value(bool has_ui) noexcept{
				this->has_ui = has_ui;
				return {};
			}

			static void return_void() noexcept{
			}

			[[noreturn]] static void unhandled_exception() noexcept{
				std::terminate();
			}


		private:
			friend ui_build_handle;

		};

		void resume() const{
			hdl.resume();
		}

		[[nodiscard]] bool done() const noexcept{
			return hdl.done();
		}

		ui_build_handle(const ui_build_handle& other) = delete;

		ui_build_handle(ui_build_handle&& other) noexcept
			: hdl{std::exchange(other.hdl, {})}{
		}

		ui_build_handle& operator=(const ui_build_handle& other) = delete;

		ui_build_handle& operator=(ui_build_handle&& other) noexcept{
			if(this == &other) return *this;
			dstry();
			hdl = std::exchange(other.hdl, {});
			return *this;
		}

		~ui_build_handle(){
			dstry();
		}


		[[nodiscard]] bool has_ui() const{
			return hdl.promise().has_ui;
		}

	private:
		void dstry() noexcept{
			if(hdl){
				if(!done()){
					resume();
				}
				hdl.destroy();
			}
		}

		handle hdl{};
	};

	export
	enum struct category{
		misc,
		weaponry,
		utility,
		maneuvering,
		powering,
		defensive,

		LAST
	};

	export constexpr inline std::array<std::string_view, std::to_underlying(category::LAST)>
		category_names{
			NAME_OF(misc),
			NAME_OF(weaponry),
			NAME_OF(utility),
			NAME_OF(maneuvering),
			NAME_OF(powering),
			NAME_OF(defensive),
		};


	export
	struct energy_consumer_comp{
		unsigned max_energy_consumption;
		bool reserve_energy_if_power_off;
	};

	export struct basic_chamber;

	struct chamber_instance_data{
		virtual ~chamber_instance_data() = default;

		virtual void install(/*building*/){

		}

		virtual srl::chunk_serialize_handle write(std::ostream& stream) const{
			co_yield 0;
		}

		virtual void read(std::ispanstream& bounded_stream){

		}


		virtual void draw(
			const basic_chamber& meta,
			math::frect region,
			graphic::renderer_ui& renderer_ui,
			const graphic::camera2& camera
		) const{

		}

		virtual ui_build_handle build_ui(ui::table& table, ui_edit_context& context){
			co_yield false;
		}

		// virtual void mouse_events()
	};

	// export
	// using
	struct basic_chamber{
		std::string_view name;
		category category;

		math::usize2 extent;
		ecs::static_hit_point hit_point;

		bool has_placement_direction;
		bool is_pure_static;

		virtual ~basic_chamber() = default;

		[[nodiscard]] virtual std::optional<math::vec2> get_part_offset() const noexcept{
			return std::nullopt;
		}

		[[nodiscard]] virtual std::optional<float> get_part_direction() const noexcept{
			return std::nullopt;
		}

		[[nodiscard]] virtual std::optional<math::range> get_radius_range() const noexcept{
			return std::nullopt;
		}

		[[nodiscard]] virtual std::optional<math::range> get_range_angular() const noexcept{
			return std::nullopt;
		}

		[[nodiscard]] virtual std::optional<energy_consumer_comp> get_energy_consumption() const noexcept{
			return std::nullopt;
		}

		virtual void draw(math::frect region, graphic::renderer_ui& renderer_ui, const graphic::camera2& camera) const{

		}

		virtual void draw(graphic::renderer_world& renderer_world) const{

		}

		virtual int get_energy_usage() const noexcept{
			return get_energy_consumption().transform([](energy_consumer_comp&& c){return -c.max_energy_consumption;}).value_or(0);
		}



		virtual void build_ui(ui::table& table) const;

		template <typename S>
		auto create_instance_data(this const S& self)  {
			return std::unique_ptr<std::remove_pointer_t<decltype(std::declval<const S&>().create_instance_data_impl())>>{self.create_instance_data_impl()};
		}

	protected:
		[[nodiscard]] virtual owner<chamber_instance_data*> create_instance_data_impl() const{
			return nullptr;
		}

		template <typename S>
		std::optional<energy_consumer_comp> deduced_get_energy_consumption(this const S& self){
			if constexpr (std::derived_from<S, energy_consumer_comp>){
				return static_cast<const energy_consumer_comp&>(self);
			}else{
				return std::nullopt;
			}
		}
	};

	export
	struct armor : basic_chamber{

	};

	export
	struct energy_generator : basic_chamber{
		unsigned max_energy_generation{};

		[[nodiscard]] int get_energy_usage() const noexcept override{
			return max_energy_generation;
		}
	};

	template <std::derived_from<basic_chamber> T>
	using chamber_meta_instance_data_t = decltype(std::declval<const T&>().create_instance_data())::element_type;

	//
	// export
	// struct superstructure


	export
	struct turret_base : basic_chamber, energy_consumer_comp{
		friend basic_chamber;

		struct turret_instance_data : chamber_instance_data{
			float rotation;
		};

		ecs::drawer::part_pos_transform transform{};
		float rotate_torque{};
		float shooting_field_angle{};

		[[nodiscard]] std::optional<math::vec2> get_part_offset() const noexcept override{
			return transform.vec;
		}
		[[nodiscard]] std::optional<energy_consumer_comp> get_energy_consumption() const noexcept override{
			return deduced_get_energy_consumption();
		}

	protected:
		[[nodiscard]] owner<turret_instance_data*> create_instance_data_impl() const override{
			return new turret_instance_data{};
		}
	};


	export
	struct radar : basic_chamber, energy_consumer_comp{
		friend basic_chamber;

		ecs::drawer::part_pos_transform transform{};

		math::range targeting_range_radius{};
		math::range targeting_range_angular{-math::pi, math::pi};
		float reload_duration{};

		[[nodiscard]] std::optional<math::vec2> get_part_offset() const noexcept override{
			return transform.vec;
		}

		[[nodiscard]] std::optional<energy_consumer_comp> get_energy_consumption() const noexcept override{
			return deduced_get_energy_consumption();
		}

		void draw(math::frect region, graphic::renderer_ui& renderer_ui, const graphic::camera2& camera) const override;


		struct radar_instance_data : chamber_instance_data{
			float rotation;

			void draw(const basic_chamber& meta, math::frect region, graphic::renderer_ui& renderer_ui, const graphic::camera2& camera) const override;

			ui_build_handle build_ui(ui::table& table, ui_edit_context& context) override;


		};

	protected:
		[[nodiscard]] owner<radar_instance_data*> create_instance_data_impl() const override{
			return new radar_instance_data{};
		}
	};


	export using chamber_types = std::tuple<
		turret_base
	, radar
	, armor
	, energy_generator
	>;


	// export
	// struct chamber_interpreter{
	// 	[[nodiscard]] constexpr chamber_interpreter() = default;
	//
	// 	constexpr virtual ~chamber_interpreter() = default;
	//
	// 	[[nodiscard]] virtual const basic_chamber& get(const basic_chamber& info) const noexcept{
	// 		return info;
	// 	}
	//
	// 	[[nodiscard]] virtual std::optional<math::trans2> get_part_trans(const basic_chamber& info) const noexcept{
	// 		return std::nullopt;
	// 	}
	//
	// 	[[nodiscard]] virtual std::optional<math::range> get_radius_range(const basic_chamber& info) const noexcept{
	// 		return std::nullopt;
	// 	}
	//
	// 	[[nodiscard]] virtual std::optional<math::range> get_range_angular(const basic_chamber& info) const noexcept{
	// 		return std::nullopt;
	// 	}
	//
	// 	[[nodiscard]] virtual std::optional<energy_consumer_comp> get_energy_consumption(const basic_chamber& info) const noexcept{
	// 		return std::nullopt;
	// 	}
	//
	// protected:
	// 	template <typename S>
	// 	std::optional<energy_consumer_comp> duduced_get_energy_consumption(this const S& self, const basic_chamber& info){
	// 		auto& dinfo = self.get(info);
	// 		using Ty = std::remove_cvref_t<decltype(dinfo)>;
	// 		if constexpr (std::derived_from<Ty, energy_consumer_comp>){
	// 			return static_cast<const energy_consumer_comp&>(dinfo);
	// 		}else{
	// 			return std::nullopt;
	// 		}
	// 	}
	// };
	//
	// export
	// template <typename T>
	// struct chamber_interpreter_of : chamber_interpreter{};
	//
	//
	//
	//
	// export
	// template <>
	// struct chamber_interpreter_of<turret_base> final : chamber_interpreter{
	// 	[[nodiscard]] const turret_base& get(const basic_chamber& info) const noexcept override{
	// 		return static_cast<const turret_base&>(info);
	// 	}
	//
	// 	[[nodiscard]] std::optional<math::trans2> get_part_trans(const basic_chamber& info) const noexcept override{
	// 		return get(info).transform;
	// 	}
	//
	// 	[[nodiscard]] std::optional<math::range> get_range_angular(const basic_chamber& info) const noexcept override{
	// 		return math::range{-math::pi, math::pi};
	// 	}
	//
	// 	[[nodiscard]] std::optional<energy_consumer_comp> get_energy_consumption(const basic_chamber& info) const noexcept override{
	// 		return duduced_get_energy_consumption(info);
	// 	}
	// };
	//
	//
	//
	// export
	// template <>
	// struct chamber_interpreter_of<radar> final : chamber_interpreter{
	// 	[[nodiscard]] const radar& get(const basic_chamber& info) const noexcept override{
	// 		return static_cast<const radar&>(info);
	// 	}
	//
	// 	[[nodiscard]] std::optional<math::trans2> get_part_trans(const basic_chamber& info) const noexcept override{
	// 		return get(info).transform;
	// 	}
	//
	// 	[[nodiscard]] std::optional<math::range> get_range_angular(const basic_chamber& info) const noexcept override{
	// 		return get(info).targeting_range_angular;
	// 	}
	//
	// 	std::optional<math::range> get_radius_range(const basic_chamber& info) const noexcept override{
	// 		return get(info).targeting_range_radius;
	// 	}
	//
	// 	[[nodiscard]] std::optional<energy_consumer_comp> get_energy_consumption(const basic_chamber& info) const noexcept override{
	// 		return duduced_get_energy_consumption(info);
	// 	}
	// };
	//
	//
	//
	//
	//
	// using chamber_type_to_interpreter = const chamber_interpreter&(*)() noexcept;
	// static constexpr inline chamber_interpreter def;
	//
	// using chamber_interpreter_factory_type = type_map<chamber_type_to_interpreter, chamber_types>;
	//
	// template <typename T>
	// struct getter{
	// 	static constexpr chamber_interpreter_of<T> interpreter{};
	// 	static constexpr const chamber_interpreter& operator()() noexcept{
	// 		return interpreter;
	// 	}
	// };
	//
	// export constexpr inline chamber_interpreter_factory_type chamber_interpreter_factory = []{
	// 	chamber_interpreter_factory_type f{};
	// 	[&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
	// 		([&]<std::size_t I>(){
	// 			using T = std::tuple_element_t<I, chamber_types>;
	// 			f.at<T>() = std::addressof(getter<T>::operator());
	// 		}.template operator()<Idx>(), ...);
	//
	// 	}(std::make_index_sequence<std::tuple_size_v<chamber_types>>{});
	// 	return f;
	// }();
}

