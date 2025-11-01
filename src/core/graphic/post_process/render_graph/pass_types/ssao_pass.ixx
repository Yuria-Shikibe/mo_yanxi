
export module mo_yanxi.graphic.render_graph.ssao;

export import mo_yanxi.graphic.render_graph.post_process_pass;
import mo_yanxi.vk;
import mo_yanxi.math.vector2;
import std;

namespace mo_yanxi::graphic::render_graph{

	struct ssao_kernal_block {
		struct unit{
			math::vec2 off{};
			float weight{};
			float cap{};
		};

		struct kernal_info {
			std::size_t count{};
			float distance{};
			float weight{};
		};

		static constexpr std::array SSAO_Params{
			kernal_info{16, 0.15702702700f, 0},
			kernal_info{12, 0.32702702700f, 0},
			kernal_info{8, 0.55062162162f, 0},
			kernal_info{4, 0.83062162162f, 0},
		};

		static constexpr std::size_t def_kernal_size{[]{
			std::size_t rst{};

			for (const auto & ssao_param : SSAO_Params) {
				rst += ssao_param.count;
			}

			return rst;
		}()};

		[[nodiscard]] constexpr ssao_kernal_block() = default;

		[[nodiscard]] constexpr explicit ssao_kernal_block(
			const math::isize2 size, const float scl = 4.25f) : scale(scl){
			const auto screen_scale = ~size.as<float>();

			for(std::size_t count{}; const auto& param : SSAO_Params){
				for(std::size_t i = 0; i < param.count; ++i){
					kernal[count].off.set_polar_deg(
						(360.f / static_cast<float>(param.count)) * static_cast<float>(i), param.distance) *= screen_scale;
					kernal[count].weight = param.weight;
					count++;
				}
			}
		}

		static constexpr std::size_t kernal_max_size = 64;

		static_assert(def_kernal_size <= kernal_max_size);

		std::array<unit, kernal_max_size> kernal{};
		std::int32_t kernalSize{def_kernal_size};
		float scale{};
	};

	export
	struct ssao_pass final : post_process_stage{
		using post_process_stage::post_process_stage;


		void set_scale(const float scale){
			vk::buffer_mapper{ubo()}.load(scale, std::bit_cast<std::uint32_t>(&ssao_kernal_block::scale));
			this->scale = scale;
		}

	private:
		float scale{4};

		void reset_resources(vk::context& context, const pass_resource_reference& resources,
			const math::u32size2 extent) override{
			post_process_stage::reset_resources(context, resources, extent);

			vk::buffer_mapper{ubo()}.load(ssao_kernal_block{extent.as<math::isize2>(), scale});
		}
	};
}
