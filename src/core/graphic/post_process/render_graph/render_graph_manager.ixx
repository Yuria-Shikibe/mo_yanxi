module;

#include <cassert>
#include <vulkan/vulkan.h>
#include "plf_hive.h"
#include "gch/small_vector.hpp"

export module mo_yanxi.graphic.render_graph.manager;

export import mo_yanxi.graphic.render_graph.resource_desc;
import mo_yanxi.math.vector2;
import mo_yanxi.vk.context;
import mo_yanxi.vk.command_buffer;
import mo_yanxi.basic_util;
import std;

//TODO using include when the conpiler not ice



namespace mo_yanxi::graphic::render_graph{
	export
	struct pass_data;

	export
	struct pass_dependency{
		pass_data* id;
		inout_index src_idx;
		inout_index dst_idx;
	};

	struct pass_identity{
		pass_data* where{nullptr};
		pass_data* external_target{nullptr};
		inout_index slot_in{no_slot};
		inout_index slot_out{no_slot};

		const pass_data* operator->() const noexcept{
			return where;
		}

		bool operator==(const pass_identity& i) const noexcept{
			if(where == i.where){
				if(where == nullptr){
					return external_target == i.external_target && slot_out == i.slot_out && slot_in == i.slot_in;
				}else{
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] std::string format(std::string_view endpoint_name = "Tml") const;
	};


	struct resource_trace{
		resource_desc::resource_requirement requirement{};

		resource_desc::ownership_type ownership{};
		resource_desc::resource_entity *used_entity{nullptr};
		std::vector<pass_identity> passed_by{};

		[[nodiscard]] explicit resource_trace(const resource_desc::resource_requirement& requirement)
			: requirement(requirement){
		}

		[[nodiscard]] pass_identity get_head() const noexcept{
			return passed_by.front();
		}

		[[nodiscard]] inout_index source_out() const noexcept{
			return get_head().slot_out;
		}

		[[nodiscard]] bool from_external() const noexcept{
			return get_head().where == nullptr;
		}

		void update_ownership(resource_desc::ownership_type type){
			if(std::to_underlying(type) >= std::to_underlying(ownership)){
				ownership = type;
			}else{
				throw std::runtime_error("incompatible ownership type");
			}
		}
	};

}

template <>
struct std::hash<mo_yanxi::graphic::render_graph::pass_identity>{
	static std::size_t operator()(const mo_yanxi::graphic::render_graph::pass_identity& idt) noexcept{
		if(idt.where){
			auto p0 = std::bit_cast<std::uintptr_t>(idt.where);
			return std::hash<std::size_t>{}(p0);
		}else{
			auto p1 = std::bit_cast<std::size_t>(idt.external_target) ^ static_cast<std::size_t>(idt.slot_out) ^ (static_cast<std::size_t>(idt.slot_in) << 31);
			return std::hash<std::size_t>{}(p1);
		}
	}
};

namespace mo_yanxi::graphic::render_graph{


	namespace post_graph{
		struct record_pair{
			inout_index index{};
			resource_trace* life_bound{};
		};
	}


	export
	struct render_graph_manager;

	export
	struct pass_data;

	export
	struct pass_resource_reference{
		friend render_graph_manager;

	private:
		gch::small_vector<const resource_desc::resource_entity*, 4> inputs{};
		gch::small_vector<const resource_desc::resource_entity*, 2> outputs{};

	public:
		[[nodiscard]] bool has_in(inout_index index) const noexcept{
			if(index >= inputs.size()) return false;
			return inputs[index] != nullptr;
		}

		[[nodiscard]] bool has_out(inout_index index) const noexcept{
			if(index >= outputs.size()) return false;
			return outputs[index] != nullptr;
		}

		[[nodiscard]] const resource_desc::resource_entity* get_in_if(inout_index index) const noexcept{
			if(index >= inputs.size()) return nullptr;
			return inputs[index];
		}

		[[nodiscard]] const resource_desc::resource_entity* get_out_if(inout_index index) const noexcept{
			if(index >= outputs.size()) return nullptr;
			return outputs[index];
		}

		[[nodiscard]] const resource_desc::resource_entity& at_in(inout_index index) const {
			if(auto res = get_in_if(index)){
				return *res;
			}else{
				throw std::out_of_range("resource not found");
			}
		}

		[[nodiscard]] const resource_desc::resource_entity& at_out(inout_index index) const {
			if(auto res = get_out_if(index)){
				return *res;
			}else{
				throw std::out_of_range("resource not found");
			}
		}

		void set_in(inout_index idx, const resource_desc::resource_entity* res){
			inputs.resize(std::max<std::size_t>(idx + 1, inputs.size()));
			assert(inputs[idx] == nullptr);
			for (const auto & input : inputs){
				assert(input != res);
			}

			inputs[idx] = res;

		}

		void set_out(inout_index idx, const resource_desc::resource_entity* res){
			outputs.resize(std::max<std::size_t>(idx + 1, outputs.size()));
			assert(outputs[idx] == nullptr);
			// for (const auto & input : inputs){
			// 	assert(input != res);
			// }

			outputs[idx] = res;
		}

		void set_inout(inout_index idx, const resource_desc::resource_entity* res){
			set_in(idx, res);
			set_out(idx, res);
		}
	};

	export
	struct pass_meta{
		friend pass_data;
		friend render_graph_manager;

		static constexpr math::u32size2 compute_group_unit_size2{16, 16};

		constexpr static math::u32size2 get_work_group_size(math::u32size2 image_size) noexcept{
			return image_size.add(compute_group_unit_size2.copy().sub(1u, 1u)).div(compute_group_unit_size2);
		}

		[[nodiscard]] explicit pass_meta(vk::context& context){

		}

		virtual ~pass_meta() = default;


	protected:
		[[nodiscard]] virtual const pass_inout_connection& sockets() const noexcept = 0;

		virtual void post_init(vk::context& context, const math::u32size2 extent){

		}

		virtual void record_command(
			vk::context& context,
			const pass_resource_reference& resources,
			math::u32size2 extent,
			VkCommandBuffer buffer){

		}

		virtual void reset_resources(vk::context& context, const pass_resource_reference& resources, const math::u32size2 extent){

		}


		[[nodiscard]] virtual std::span<const inout_index> get_required_internal_buffer_slots() const noexcept{
			return {};
		}

		[[nodiscard]] virtual std::string_view get_name() const noexcept{
			return {};
		}
	};

	export
	struct explicit_resource_desc{
		inout_index slot;
		bool shared;
	};

	export
	struct pass_data{
		gch::small_vector<pass_dependency> dependencies_resource_{};
		gch::small_vector<pass_data*> dependencies_executions_{};

		gch::small_vector<resource_desc::explicit_resource_usage> external_inputs_{};
		gch::small_vector<resource_desc::explicit_resource_usage> external_outputs_{};
		pass_resource_reference used_resources_{};

		std::unique_ptr<pass_meta> meta{};

		[[nodiscard]] explicit pass_data(std::unique_ptr<pass_meta>&& meta)
			: meta(std::move(meta)){
		}

		[[nodiscard]] std::string get_identity_name() const noexcept{
			return std::format("({:#X}){}", static_cast<std::uint16_t>(std::bit_cast<std::uintptr_t>(this)), meta->get_name());
		}

		[[nodiscard]] const pass_inout_connection& sockets() const noexcept{
			return meta->sockets();
		}

		void add_dep(const pass_dependency dep){
			dependencies_resource_.push_back(dep);
		}

		void add_dep(const std::initializer_list<pass_dependency> dep){
			dependencies_resource_.append(dep);
		}

		void add_exec_dep(pass_data* dep){
			dependencies_executions_.push_back(dep);
		}

		void add_exec_dep(const std::initializer_list<pass_data*> dep){
			dependencies_executions_.append(dep);
		}

		void add_output(const std::initializer_list<resource_desc::explicit_resource_usage> externals){
			external_outputs_.append(externals);
		}

		void add_input(const std::initializer_list<resource_desc::explicit_resource_usage> externals){
			external_inputs_.append(externals);
		}

		void add_inout(const std::initializer_list<resource_desc::explicit_resource_usage> externals){
			external_inputs_.append(externals);
			external_outputs_.append(externals);
		}

		auto& at_out(inout_index slot) const {
			return used_resources_.at_out(slot);
		}
	};

	struct life_trace_group{
	private:
		std::vector<resource_trace> bounds{};

		/**
		 * @brief Merge traces with the same input source
		 */
		void unique(){
			auto& range = bounds;

			std::unordered_map<pass_identity, resource_trace*> checked_outs{};
			auto itr = std::ranges::begin(range);
			auto end = std::ranges::end(range);

			while(itr != end){
				auto& indices = checked_outs[itr->get_head()];

				if(indices != nullptr){
					const auto& view = itr->passed_by;
					const auto mismatch = std::ranges::mismatch(
						indices->passed_by, view);
					for(auto& pass : std::ranges::subrange{mismatch.in2, view.end()}){
						indices->passed_by.push_back(pass);
					}

					assert(!indices->requirement.get_incompatible_info(itr->requirement).has_value());
					indices->requirement.promote(itr->requirement);

					itr = range.erase(itr);
					end = std::ranges::end(range);
				}else{
					if(itr->source_out() != no_slot) indices = &*itr;
					++itr;
				}
			}
		}

	public:

		[[nodiscard]] life_trace_group() = default;

		template <std::ranges::input_range T>
			requires (std::convertible_to<std::ranges::range_reference_t<T&&>, const pass_data&>)
		[[nodiscard]] life_trace_group(T&& pass_execute_sequence){
			for(pass_data& pass : pass_execute_sequence){
				auto& inout = pass.sockets();
				auto inout_indices = inout.get_inout_indices();

				for(const auto& entry : pass.external_outputs_){
					auto& rst = bounds.emplace_back(inout.at_out(entry.slot));
					rst.ownership = entry.get_ownership();

					rst.passed_by.push_back({nullptr, &pass, entry.slot, no_slot});
					auto& next = rst.passed_by.emplace_back(&pass, nullptr, no_slot, entry.slot);

					if(auto itr =
							std::ranges::find(inout_indices, entry.slot, &slot_pair::out);
						itr != inout_indices.end()){
						next.slot_in = itr->in;
					}
				}

				for(std::size_t i = 0; i < inout.input_slots.size(); ++i){
					if(std::ranges::contains(inout_indices, i, &slot_pair::in)) continue;

					auto& rst = bounds.emplace_back(inout.at_in(i));
					auto& cur = rst.passed_by.emplace_back(&pass);
					cur.slot_in = i;
				}
			}


			{
				auto is_done = [](const resource_trace& b){
					return b.passed_by.back().slot_in == no_slot;
				};


				for(auto&& res_bnd : bounds){
					if(is_done(res_bnd)) continue;

					auto cur_head = res_bnd.passed_by.back();
					while(true){
						for(const auto& dependency : cur_head->dependencies_resource_){
							if(dependency.dst_idx == cur_head.slot_in){
								auto& pass = *dependency.id;
								auto& inout = pass.sockets();
								auto indices = inout.get_inout_indices();

								res_bnd.requirement.promote(inout.at_out(dependency.src_idx));

								auto& cur = res_bnd.passed_by.emplace_back(&pass);
								cur.slot_out = dependency.src_idx;

								if(auto itr =
										std::ranges::find(
											indices, dependency.src_idx, &slot_pair::out);
									itr != indices.end()){
									cur.slot_in = itr->in;
								}

								break;
							}
						}

						if(cur_head == res_bnd.passed_by.back()){
							break;
						}
						cur_head = res_bnd.passed_by.back();
					}

					if(is_done(res_bnd)) continue;

					assert(cur_head.where != nullptr);
					for(const auto& entry : cur_head.where->external_inputs_){
						if(entry.slot == cur_head.slot_in){
							res_bnd.passed_by.push_back(pass_identity{nullptr, cur_head.where, no_slot, entry.slot});
							res_bnd.update_ownership(entry.get_ownership());

							break;
						}
					}
				}
			}

			for(auto& lifebound : bounds){
				std::ranges::reverse(lifebound.passed_by);
			}

			unique();
		}

		auto begin(this auto& self) noexcept{
			return std::ranges::begin(self.bounds);
		}

		auto end(this auto& self) noexcept{
			return std::ranges::end(self.bounds);
		}

		auto size() const noexcept{
			return bounds.size();
		}
	};

	export
	template <std::derived_from<pass_meta> T>
	struct add_result{
		pass_data& pass;
		T& meta;

		[[nodiscard]] pass_data* id() const noexcept{
			return std::addressof(pass);
		}
	};

	struct render_graph_manager{
	private:
		vk::context* context_{};

		plf::hive<pass_data> passes_{};
		std::vector<pass_data*> execute_sequence_{};

		std::vector<resource_desc::resource_entity> shared_resources{};
		std::vector<resource_desc::resource_entity> exclusive_resources{};
		std::vector<resource_desc::resource_entity> borrowed_resources{};

		plf::hive<resource_desc::explicit_resource> explicit_resources{};

		math::u32size2 extent_{};

		vk::command_buffer main_command_buffer{};
		life_trace_group life_bounds_{};

	public:
		[[nodiscard]] render_graph_manager() = default;

		[[nodiscard]] explicit render_graph_manager(vk::context& context)
			: context_(&context), main_command_buffer{context_->get_compute_command_pool().obtain()}{
		}

		template <std::derived_from<pass_meta> T, typename ...Args>
			requires (std::constructible_from<T, vk::context&, Args&& ...>)
		add_result<T> add_stage(Args&& ...args){
			pass_data& pass = *passes_.insert(pass_data{std::make_unique<T>(*context_, std::forward<Args>(args) ...)});
			return {pass, static_cast<T&>(*pass.meta)};
		}

		auto& add_explicit_resource(resource_desc::explicit_resource res){
			return *explicit_resources.insert(std::move(res));
		}

		void sort(){
			if(passes_.empty()){
				throw std::runtime_error("No outputs found");
			}

			struct node{
				unsigned in_degree{};
			};
			std::unordered_map<pass_data*, node> nodes;



			for(auto& v : passes_){
				nodes.try_emplace(&v);

				for(auto& dep : v.dependencies_resource_){
					++nodes[dep.id].in_degree;
				}

				for(auto& dep : v.dependencies_executions_){
					++nodes[dep].in_degree;
				}
			}

			std::vector<pass_data*> queue;
			queue.reserve(passes_.size() * 2);
			execute_sequence_.reserve(passes_.size());

			std::size_t current_index{};

			{//Set Initial Nodes
				for (const auto & [node, indeg] : nodes){
					if(indeg.in_degree)continue;

					queue.push_back(node);
				}
			}

			if(queue.empty()){
				throw std::runtime_error("No zero dependes found");
			}

			while(current_index != queue.size()){
				auto current = queue[current_index++];
				execute_sequence_.push_back(current);

				for(const auto& dependency : current->dependencies_resource_){
					auto& deg = nodes[dependency.id].in_degree;
					--deg;
					if(deg == 0){
						queue.push_back(dependency.id);
					}
				}
				for(const auto& dependency : current->dependencies_executions_){
					auto& deg = nodes[dependency].in_degree;
					--deg;
					if(deg == 0){
						queue.push_back(dependency);
					}
				}
			}

			if(execute_sequence_.size() != passes_.size()){
				throw std::runtime_error("RING detected in execute graph: sorted_tasks_.size() != nodes.size()");
			}

			std::ranges::reverse(execute_sequence_);
		}

		void check_sockets_connection() const{
			for(const auto& stage : execute_sequence_ | ranges::views::deref){
				auto& inout = stage.sockets();

				std::unordered_map<inout_index, const resource_desc::resource_requirement*> resources{inout.input_slots.size()};
				for(const auto& [idx, data_idx] : inout.input_slots | std::views::enumerate){
					resources.try_emplace(idx, std::addressof(inout.data[data_idx]));
				}

				for(const auto& dependency : stage.dependencies_resource_){
					if(auto itr = resources.find(dependency.dst_idx); itr != resources.end()){
						auto& dep_out = dependency.id->sockets();
						if(auto src_data = dep_out.get_out(dependency.src_idx)){
							if(auto cpt = src_data->get_incompatible_info(*itr->second)){
								std::println(std::cerr, "{} {} out[{}] -> {} in[{}]",
									cpt.value(),
										dependency.id->get_identity_name(), dependency.src_idx,
										stage.get_identity_name(), dependency.dst_idx
								);
							} else{

								resources.erase(itr);
							}
						} else{
							std::println(std::cerr, "Unmatched Slot {} out[{}] -> {} in[{}]",
							             dependency.id->get_identity_name(), dependency.src_idx,
							             stage.get_identity_name(), dependency.dst_idx
							);
						}
					}
				}

				for(const auto& res : stage.external_inputs_){
					if(auto ritr = resources.find(res.slot); ritr != resources.end()){
						if(res.is_local()){
							resources.erase(ritr);
							continue;
						}

						if(ritr->second->type() == res.resource->type()){
							resources.erase(ritr);
						} else{
							//TODO type mismatch error
						}
					} else{
						//TODO slot mismatch error
					}
				}

				for(const auto& [in_idx, desc] : resources){
					std::println(std::cerr, "Unresolved Slot {} in[{}]", stage.get_identity_name(), in_idx
					);
				}
			}
		}

	public:
		void analysis_minimal_allocation(){
			life_bounds_ = life_trace_group{execute_sequence_ | std::views::reverse | ranges::views::deref};

			using namespace post_graph;
			using namespace resource_desc;

			auto& life_bounds = life_bounds_;
			static constexpr resource_requirement_partial_greater_comp comp{};
			std::ranges::sort(life_bounds, comp, &resource_trace::requirement);

			std::vector<unsigned> lifebound_assignments(life_bounds.size(), no_slot);

			struct partition{
				std::vector<resource_requirement> requirements{};
				std::unordered_map<const pass_data*, std::vector<std::uint8_t>> stage_captures{};
				std::vector<unsigned> indices{};

				unsigned prefix_sum{};

				[[nodiscard]] const resource_requirement& identity() const noexcept{
					return requirements.front();
				}

				std::size_t add(const resource_requirement& requirement){
					const auto idx = requirements.size();
					indices.push_back(idx);
					requirements.emplace_back(requirement);
					for (auto& marks : stage_captures | std::views::values){
						marks.resize(requirements.size(), 0);
					}
					return idx;
				}
			};

			std::vector<partition> minimal_requirements_per_partition{};

			auto get_partition_itr = [&](const resource_requirement& r){
				return std::ranges::find_if(minimal_requirements_per_partition, [&](const partition& req){
					return !req.identity().get_incompatible_info(r).has_value();
				});
			};


			{
				for (auto&& [idx, image] : life_bounds | std::views::enumerate){
					if(image.ownership != ownership_type::shared)continue;

					if(auto category = get_partition_itr(image.requirement); category == minimal_requirements_per_partition.end()){
						auto& vec = minimal_requirements_per_partition.emplace_back();
						for (const auto& pass : execute_sequence_){
							vec.stage_captures.try_emplace(pass);
						}

						vec.add(image.requirement);
					}
				}

				for (auto&& [bound, assignment] : std::views::zip(life_bounds, lifebound_assignments)){
					if(bound.ownership != ownership_type::shared)continue;

					if(assignment != no_slot)continue;

					auto partition_itr = get_partition_itr(bound.requirement);
					assert(partition_itr != minimal_requirements_per_partition.end());
					std::ranges::sort(partition_itr->indices, [&](unsigned l, unsigned r){
						return comp(partition_itr->requirements[l], partition_itr->requirements[r]);
					});
					for (auto& candidate : partition_itr->indices){
						if(std::ranges::any_of(bound.passed_by, [&](const pass_identity& passed_by){
							return passed_by.where != nullptr && partition_itr->stage_captures.at(passed_by.where)[candidate];
						})){
							continue;
						}

						if(comp(bound.requirement, partition_itr->requirements[candidate])){
							for (const auto & passed_by : bound.passed_by){
								if(passed_by.where == nullptr)continue;
								partition_itr->stage_captures.at(passed_by.where)[candidate] = true;
							}
							assignment = candidate;

							goto skip;
						}
					}

					//No currently compatibled, promote the minimal found one to fit
					for (unsigned candidate : partition_itr->indices | std::views::reverse){
						if(std::ranges::any_of(bound.passed_by, [&](const pass_identity& passed_by){
							return passed_by.where != nullptr && partition_itr->stage_captures.at(passed_by.where)[candidate];
						})){
							continue;
						}

						for (const auto & passed_by : bound.passed_by){
							if(passed_by.where == nullptr)continue;
							partition_itr->stage_captures.at(passed_by.where)[candidate] = true;
						}

						assignment = candidate;
						partition_itr->requirements[candidate].promote(bound.requirement);
						goto skip;

					}

					assignment = partition_itr->add(bound.requirement);

					skip:
					(void)0;
				}
			}

			{
				unsigned prefix_sum{};
				for (auto & pt : minimal_requirements_per_partition){
					pt.prefix_sum = prefix_sum;
					prefix_sum += pt.requirements.size();

					for (const auto& requirement : pt.requirements){
						shared_resources.push_back(resource_entity{requirement});
					}
				}
			}

			for (auto&& [bound, assignment] : std::views::zip(life_bounds, lifebound_assignments)){
				if(bound.ownership != ownership_type::shared)continue;
				assert(assignment != no_slot);

				auto& pt = *get_partition_itr(bound.requirement);
				bound.used_entity = &shared_resources[pt.prefix_sum + assignment];

				for (const auto & passed_by : bound.passed_by){
					if(!passed_by.where)continue;
					auto& ref = passed_by.where->used_resources_;
					if(passed_by.slot_in != no_slot){
						ref.set_in(passed_by.slot_in, bound.used_entity);
					}

					if(passed_by.slot_out != no_slot){
						ref.set_out(passed_by.slot_out, bound.used_entity);
					}
				}
			}

			{
				//assign exclusive resources
				for (const auto & req : life_bounds){
					if(req.ownership != ownership_type::exclusive)continue;
					exclusive_resources.push_back(resource_entity{req.requirement});
				}

				for (const auto & [idx, req] : life_bounds
					| std::views::filter([](const resource_trace& b){return b.ownership == ownership_type::exclusive;})
					| std::views::enumerate){

					req.used_entity = &exclusive_resources[idx];

					for (const auto & passed_by : req.passed_by){
						if(!passed_by.where)continue;
						auto& ref = passed_by.where->used_resources_;

						if(passed_by.slot_in != no_slot){
							ref.set_in(passed_by.slot_in, req.used_entity);
						}

						if(passed_by.slot_out != no_slot){
							ref.set_out(passed_by.slot_out, req.used_entity);
						}
					}
				}
			}

			{

				std::unordered_map<const explicit_resource*, resource_entity*> group;
				std::unordered_map<pass_identity, resource_entity* *> pass_to_group;

				for (auto& pass : passes_){
					for (const auto & re : pass.external_inputs_){
						if(re.is_local())continue;

						pass_to_group[{nullptr, &pass, no_slot, re.slot}] = &(group[re.resource] = nullptr);
					}
				}

				borrowed_resources.reserve(group.size());

				for (const auto & [idx, req] : life_bounds
					| std::views::filter([](const resource_trace& b){return b.ownership == ownership_type::external;})
					| std::views::enumerate){
					if(const auto ptr = pass_to_group.at(req.get_head()); *ptr != nullptr){
						req.used_entity = *ptr;
					}else{
						*ptr = req.used_entity = &borrowed_resources.emplace_back(resource_entity{req.requirement});
					}

					for (const auto & passed_by : req.passed_by){
						if(!passed_by.where)continue;
						auto& ref = passed_by.where->used_resources_;

						if(passed_by.slot_in != no_slot){
							ref.set_in(passed_by.slot_in, req.used_entity);
						}

						if(passed_by.slot_out != no_slot){
							ref.set_out(passed_by.slot_out, req.used_entity);
						}
					}
				}

				(void)0;
			}
		}

		/*void print_resource_reference() const {
			std::println();
			std::println("Resource Requirements: {}", shared_resources.size());

			std::map<const resource_desc::resource_entity*, std::vector<post_graph::pass_identity>> inouts{};
			for (const auto & [pass, ref] : resource_ref){
				for (const auto & [idx, res] : ref.inputs | std::views::enumerate){
					if(res){
						inouts[res].push_back(post_graph::pass_identity{pass, nullptr, static_cast<inout_index>(idx)});
					}
				}
				for (const auto & [idx, res] : ref.outputs | std::views::enumerate){
					if(res){
						auto& vec = inouts[res];
						if(auto itr = std::ranges::find(vec, pass, &post_graph::pass_identity::where); itr != vec.end()){
							itr->slot_out = idx;
						}else{
							vec.push_back(post_graph::pass_identity{pass, nullptr, no_slot, static_cast<inout_index>(idx)});
						}

					}
				}
			}

			for (const auto & [res, user] : inouts){
				std::println("Res[{:0>2}]<{}>: ", res - shared_resources.data(), res->overall_req.type_name());
				std::println("{:n:}", user | std::views::transform([](const post_graph::pass_identity& value){
					return value.format("None");
				}));
				std::println();
			}
		}*/

		void auto_create_buffer(){
			for (auto & stage : passes_){
				for (inout_index slot : stage.meta->get_required_internal_buffer_slots()){
					stage.add_input({{slot, true}});
				}
			}
		}


		void post_init(){
			for (auto & stage : passes_){
				stage.meta->post_init(*context_, extent_);
			}
		}

		void update_external_resources() noexcept{
			static constexpr auto get_resource = [](std::vector<resource_desc::resource_entity>& range, const resource_desc::resource_entity* ptr) -> resource_desc::resource_entity* {
				static constexpr auto lt = std::ranges::less{};
				static constexpr auto gteq = std::ranges::greater_equal{};
				if( gteq(ptr, range.data()) && lt(ptr, range.data() + range.size())){
					return range.data() + (ptr - range.data());
				}else{
					return nullptr;
				}
			};

			static constexpr auto update = [](const resource_desc::explicit_resource& external_resource, resource_desc::resource_entity& entity){
				using namespace resource_desc;
				std::visit(overload_def_noop{
					std::in_place_type<void>,
					[](const external_buffer& ext, buffer_entity& ent){
						ent.buffer = ext.handle;
					}, [](const external_image& ext, image_entity& ent){
						ent.image = ext.handle;
					}
				}, external_resource.desc, entity.resource);
			};

			for (auto & pass : passes_){
				for (const auto & [slot, entity] : pass.used_resources_.inputs | std::views::enumerate){
					if(entity == nullptr)continue;
					const auto res = get_resource(borrowed_resources, entity);
					if(!res)continue;

					for (const auto & external : pass.external_inputs_){
						if(external.slot == slot){
							assert(!external.is_local());
							update(*external.resource, *res);
							break;
						}
					}
				}

				for (const auto & [slot, entity] : pass.used_resources_.outputs | std::views::enumerate){
					if(entity == nullptr)continue;
					const auto res = get_resource(borrowed_resources, entity);
					if(!res)continue;

					for (const auto & external : pass.external_outputs_){
						if(external.slot == slot){
							assert(!external.is_local());
							update(*external.resource, *res);
							break;
						}
					}
				}
			}
		}

		void resize(math::u32size2 size){
			// if(extent_ != size){
			//TODO no force reallocat on reopen the window after minimizing causes device lost
				extent_ = size;
				allocate();
			// }
		}

		void resize(VkExtent2D size){
			resize(std::bit_cast<math::u32size2>(size));
		}
		void resize(){
			resize(context_->get_extent());
		}

		void allocate(){
			for (auto & local_resource : shared_resources){
				local_resource.allocate_image(context_->get_allocator(), std::bit_cast<VkExtent2D>(extent_));
			}

			for (auto & local_resource : exclusive_resources){
				local_resource.allocate_image(context_->get_allocator(), std::bit_cast<VkExtent2D>(extent_));
			}

			for (const auto & stage : passes_){
				stage.meta->reset_resources(*context_, stage.used_resources_, extent_);
			}
		}

		void create_command(){
			using namespace resource_desc;
			std::unordered_map<const resource_entity*, entity_state> res_states{};
			std::unordered_map<const resource_entity*, bool> already_modified_mark{};

			vk::scoped_recorder scoped_recorder{main_command_buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

			vk::cmd::dependency_gen dependency_gen{};

			for (const auto& stage : execute_sequence_ | ranges::views::deref){
				already_modified_mark.clear();

				auto& inout = stage.sockets();
				auto& ref = stage.used_resources_;

				for (const auto& [in_idx, data_idx] : inout.get_valid_in()){
					auto& rentity = ref.at_in(in_idx);


					auto& cur_req = inout.data[data_idx];

					for (const auto & res : stage.external_inputs_){
						if(res.slot == in_idx && !res.is_local()){
							auto [itr, suc] = res_states.try_emplace(ref.inputs[res.slot], entity_state{*res.resource});
							if(!suc)continue;
							auto& state = itr->second;
							state.external_init_required = true;
						}
					}


					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){
							const auto& req = cur_req.get<image_requirement>();
							VkImageLayout old_layout{};
							VkPipelineStageFlags2 old_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
							VkAccessFlags2 old_access{VK_ACCESS_2_NONE};
							const auto new_layout = req.get_expected_layout();
							if(const auto itr = res_states.find(&rentity); itr != res_states.end()){
								old_layout = itr->second.get_layout();
								old_stage = itr->second.last_stage;
								old_access = itr->second.last_access;
							}

							const auto next_stage = deduce_stage(new_layout);
							const auto next_access = req.get_image_access(next_stage);
							const auto aspect = r.get_aspect();

							dependency_gen.push(
								entity.image.image,
								old_stage,
								old_access,
								next_stage,
								next_access,
								old_layout,
								new_layout,
								{
									.aspectMask = aspect,
									.baseMipLevel = 0,
									.levelCount = value_or(r.mip_levels, 1u),
									.baseArrayLayer = 0,
									.layerCount = 1
								}
							);

							auto& mark = already_modified_mark[&rentity];
							auto& state = res_states[&rentity];
							std::get<image_entity_state>(state.desc).current_layout = new_layout;
							state.external_init_required = false;
							if(mark){
								state.last_stage |= next_stage;
								state.last_access |= next_access;
							}else{
								state.last_stage = next_stage;
								state.last_access = next_access;
								mark = true;
							}

						},
						[&](const buffer_requirement& r, const buffer_entity& entity){
							//TODO buffer barrier
						}
					}, rentity.overall_req.desc, rentity.resource);
				}


				//modify output image layout, as output initialization
				for (const auto& [out_idx, data_idx] : inout.get_valid_out()){
					auto& rentity = ref.at_out(out_idx);
					auto& cur_req = inout.data[data_idx];


					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){
							const auto& req = cur_req.get<image_requirement>();
							VkImageLayout old_layout{};
							VkPipelineStageFlags2 old_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
							VkAccessFlags2 old_access{VK_ACCESS_2_NONE};
							if(const auto itr = res_states.find(&rentity); itr != res_states.end()){
								old_layout = itr->second.get_layout();
								old_stage = itr->second.last_stage;
								old_access = itr->second.last_access;
							}

							const auto new_layout = req.get_expected_layout();
							const auto next_stage = deduce_stage(new_layout);
							const auto next_access = req.get_image_access(next_stage);
							const auto aspect = r.get_aspect();

							dependency_gen.push(
								entity.image.image,
								old_stage,
								old_access,
								next_stage,
								next_access,
								old_layout,
								new_layout,
								{
									.aspectMask = aspect,
									.baseMipLevel = 0,
									.levelCount = value_or(r.mip_levels, 1u),
									.baseArrayLayer = 0,
									.layerCount = 1
								}
							);

							auto& mark = already_modified_mark[&rentity];
							auto& state = res_states[&rentity];
							std::get<image_entity_state>(state.desc).current_layout = new_layout;
							state.external_init_required = false;
							if(mark){
								state.last_stage |= next_stage;
								state.last_access |= next_access;
							}else{
								state.last_stage = next_stage;
								state.last_access = next_access;
								mark = true;
							}

						},
						[&](const buffer_requirement& r, const buffer_entity& entity){

						}
					}, rentity.overall_req.desc, rentity.resource);
				}
				if(!dependency_gen.empty())dependency_gen.apply(scoped_recorder);

				stage.meta->record_command(*context_, ref, extent_, scoped_recorder);

				//restore output final format, transition if any should be done within the pass
				for (const auto& [out_idx, data_idx] : inout.get_valid_in()){
					auto rentity = ref.inputs[out_idx];
					auto& cur_req = inout.data[data_idx];

					assert(rentity != nullptr);

					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){
							auto layout = cur_req.get<image_requirement>().get_expected_layout_on_output();
							std::get<image_entity_state>(res_states[rentity].desc).current_layout = layout;
						},
						[&](const buffer_requirement& r, const buffer_entity& entity){

						}
					}, rentity->overall_req.desc, rentity->resource);
				}

				for (const auto& [out_idx, data_idx] : inout.get_valid_out()){
					auto rentity = ref.outputs[out_idx];
					auto& cur_req = inout.data[data_idx];

					assert(rentity != nullptr);

					std::visit(overload_narrow{
						[&](const image_requirement& r, const image_entity& entity){
							auto layout = cur_req.get<image_requirement>().get_expected_layout_on_output();
							std::get<image_entity_state>(res_states[rentity].desc).current_layout = layout;
						},
						[&](const buffer_requirement& r, const buffer_entity& entity){

						}
					}, rentity->overall_req.desc, rentity->resource);

					for (const auto & res : stage.external_outputs_){
						if(res.is_local() || res.resource->get_layout() == VK_IMAGE_LAYOUT_UNDEFINED)continue;
						if(res.slot != out_idx)continue;

						std::visit(
							overload_narrow{
								[&](const image_requirement& r, const image_entity& entity){
									VkImageLayout old_layout{};
									VkPipelineStageFlags2 old_stage{VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT};
									VkAccessFlags2 old_access{VK_ACCESS_2_NONE};
									if(const auto itr = res_states.find(rentity); itr != res_states.end()){
										old_layout = itr->second.get_layout();
										old_stage = itr->second.last_stage;
										old_access = itr->second.last_access;
									}

									const auto new_layout = res.resource->get_layout();
									const auto next_stage = value_or(res.resource->dependency.dst_stage, deduce_stage(new_layout), VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
									const auto next_access = value_or(res.resource->dependency.dst_access, deduce_external_image_access(next_stage), VK_ACCESS_2_NONE);

									const auto& req = cur_req.get<image_requirement>();
									const auto aspect = r.get_aspect();

									dependency_gen.push(
										entity.image.image,
										old_stage,
										old_access,
										next_stage,
										next_access,
										old_layout,
										new_layout,
										{
											.aspectMask = aspect,
											.baseMipLevel = 0,
											.levelCount = value_or(r.mip_levels, 1u),
											.baseArrayLayer = 0,
											.layerCount = 1
										}
									);
								},
								[&](const buffer_requirement& r, const buffer_entity& entity){

								}
							}, rentity->overall_req.desc, rentity->resource);
					}
				}
			}

			if(!dependency_gen.empty())dependency_gen.apply(scoped_recorder);
		}

		VkCommandBuffer get_main_command_buffer() const noexcept{
			return main_command_buffer;
		}

	};
}

module : private;

namespace  mo_yanxi::graphic::render_graph{
	std::string pass_identity::format(std::string_view endpoint_name) const{
		static constexpr auto fmt_slot = [](std::string_view epn, inout_index idx) -> std::string {
			return idx == no_slot ? std::string(epn) : std::format("{}", idx);
		};

		return std::format("[{}] {} [{}]", fmt_slot(endpoint_name, slot_in), std::string(where ? where->get_identity_name() : "endpoint"), fmt_slot(endpoint_name, slot_out));
	}

}
