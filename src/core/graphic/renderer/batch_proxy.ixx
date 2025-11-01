module;

#include <cassert>
#include <vulkan/vulkan.h>

export module mo_yanxi.graphic.batch_proxy;

export import mo_yanxi.vk.batch;
export import mo_yanxi.vk;

import mo_yanxi.heterogeneous.open_addr_hash;

import std;


namespace mo_yanxi::graphic{
	export
	struct command_buffer_modifier{
		struct promise_type;
		using handle = std::coroutine_handle<promise_type>;
		using value_type = VkCommandBuffer;

		[[nodiscard]] command_buffer_modifier() = default;

		[[nodiscard]] explicit command_buffer_modifier(handle&& hdl)
			: hdl{std::move(hdl)}{}

		struct promise_type{
			[[nodiscard]] promise_type() = default;

			command_buffer_modifier get_return_object(){
				return command_buffer_modifier{handle::from_promise(*this)};
			}

			[[nodiscard]] static auto initial_suspend() noexcept{ return std::suspend_never{}; }

			[[nodiscard]] static auto final_suspend() noexcept{ return std::suspend_always{}; }

			static void return_void(){}

			auto yield_value(const value_type& val) noexcept{
				toSpecify = val;
				return std::suspend_always{};
			}

			[[noreturn]] static void unhandled_exception() noexcept{
				std::terminate();
			}

		private:
			friend command_buffer_modifier;

			value_type toSpecify{};
		};

		void submit() const{
			hdl->resume();
		}

		[[nodiscard]] bool done() const noexcept{
			return hdl->done();
		}

		[[nodiscard]] decltype(auto) current_command() const noexcept{
			return hdl->promise().toSpecify;
		}

		~command_buffer_modifier(){
			if(hdl){
				assert(done());
				hdl->destroy();
			}
		}

	private:
		exclusive_handle_member<handle> hdl{};
	};

	export
	template <typename T>
	struct batch_layer_data{
		static constexpr std::size_t chunk_size = sizeof(T);
	protected:
		std::vector<T> history{};
	public:
		vk::uniform_buffer ubo{};
		T current{};

		[[nodiscard]] batch_layer_data() = default;

		[[nodiscard]] batch_layer_data(vk::context& context, std::size_t count)
			: ubo{context.get_allocator(), chunk_size * count}
		{
			history.resize(count);
			vk::buffer_mapper{ubo}.load_range(history);
		}

		void bind_to(const vk::descriptor_mapper& descriptor_mapper, const std::uint32_t binding) const{
			assert(history.size() == descriptor_mapper.base().get_chunk_count());

			for(std::size_t i = 0; i < history.size(); ++i){
				(void)descriptor_mapper.set_uniform_buffer(binding, ubo.get_address() + chunk_size * i, chunk_size, i);
			}
		}

		template <bool force = false>
		void update(std::size_t index){
			auto& last = history[index];
			if constexpr (force){
				vk::buffer_mapper{ubo}.load(
					current, sizeof(T) * index
				);
				last = current;
			}else if(last != current){
				vk::buffer_mapper{ubo}.load(
					current, sizeof(T) * index
				);
				last = current;
			}
		}
	};

	export
	struct batch_layer{
	protected:
		vk::descriptor_layout descriptor_layout{};
		vk::constant_layout constant_layout{};
		vk::pipeline_layout pipeline_layout{};
		vk::pipeline pipeline{};

		vk::descriptor_buffer descriptor_buffer{};

		std::vector<vk::command_buffer> command_buffers{};

		template <vk::contigious_range_of<VkDescriptorSetLayout> Rng = std::initializer_list<VkDescriptorSetLayout>>
		void create_pipeline(
			const vk::context& context,
			const vk::graphic_pipeline_template& pipeline_template,
			Rng&& public_sets){
			if(descriptor_layout == nullptr){
				pipeline_layout = vk::pipeline_layout{context.get_device(), 0, public_sets, constant_layout.get_constant_ranges()};
			}else{
				std::vector<VkDescriptorSetLayout> v{std::from_range, std::forward<Rng>(public_sets)};
				v.push_back(descriptor_layout.get());
				pipeline_layout = vk::pipeline_layout{context.get_device(), 0, v, constant_layout.get_constant_ranges()};
			}

			pipeline = {context.get_device(), pipeline_layout, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, pipeline_template};
		}

		void create_descriptor_buffer(vk::context& context, std::uint32_t chunk_count){
			if(descriptor_buffer != nullptr)return;

			descriptor_buffer = vk::descriptor_buffer{context.get_allocator(), descriptor_layout, descriptor_layout.binding_count(), chunk_count};
		}

	public:
		virtual ~batch_layer() = default;

		[[nodiscard]] batch_layer() = default;

		template <vk::contigious_range_of<VkDescriptorSetLayout> Rng = std::initializer_list<VkDescriptorSetLayout>>
		[[nodiscard]] batch_layer(
			vk::context& context,
			std::size_t chunk_count,
			Rng&& public_sets = {}
			){
			command_buffers.resize(chunk_count);

			for (auto && vk_command_buffer : command_buffers){
				vk_command_buffer = context.get_graphic_command_pool().obtain();
			}
		}

		virtual command_buffer_modifier create_command(
			const vk::dynamic_rendering& rendering,
			vk::batch& batch,
			// std::span<const VkDescriptorBufferBindingInfoEXT> descriptors
			VkRect2D region
		) = 0;

		virtual void try_post_data(std::size_t index){

		}

		[[nodiscard]] virtual bool requires_dump() const noexcept{
			return false;
		}

		virtual VkCommandBuffer submit(vk::context& context, std::size_t index){
			return command_buffers[index];
		}

		/**
		 * @brief
		 * @return nullptr if directly fallback to base frame
		 */
		[[nodiscard]] virtual vk::color_attachment* get_local_layer() const noexcept{
			return nullptr;
		}

		[[nodiscard]] bool has_local_layer() const noexcept{
			return get_local_layer() != nullptr;
		}

		[[nodiscard]] VkPipelineLayout get_pipeline_layout() const noexcept{
			return pipeline_layout;
		}


		virtual VkCommandBuffer get_blit_command(){
			return nullptr;
		}

		vk::descriptor_mapper get_dbo_mapper() noexcept{
			return {descriptor_buffer};
		}


		batch_layer(const batch_layer& other) = delete;
		batch_layer(batch_layer&& other) noexcept = default;
		batch_layer& operator=(const batch_layer& other) = delete;
		batch_layer& operator=(batch_layer&& other) noexcept = default;

		// virtual command_buffer_modifier t(){
		// 	vk::dynamic_rendering dynamic_rendering{};
		// 	co_yield dynamic_rendering;
		// }
	};

	template <typename T>
	concept batch_effect_layer = requires{
		requires std::derived_from<T, batch_layer>;
		{ T::layer_name } -> std::convertible_to<std::string_view>;
	};

	export
	struct batch_proxy{
		vk::batch batch{};

	protected:
		VkRect2D region{};

		string_open_addr_hash_map<std::unique_ptr<batch_layer>> layers_{};
		batch_layer* current_layer_ = nullptr;

		void get_public_sets() const noexcept = delete;
		void update_layer_commands(batch_layer& batch_layer) = delete;
	public:

		[[nodiscard]] batch_proxy() = default;

		[[nodiscard]] explicit batch_proxy(vk::batch&& batch)
			: batch(std::move(batch)){
		}

		[[nodiscard]] VkRect2D get_region() const noexcept{
			return region;
		}

		template <batch_effect_layer Lyr, std::derived_from<batch_proxy> S, typename ...Args>
			requires (std::constructible_from<Lyr, vk::context&, std::size_t, decltype(std::declval<S&>().get_public_sets()), Args...>)
		Lyr& emplace_batch_layer(this S& self, Args&& ...args){
			auto [itr, suc] = self.layers_.try_emplace(Lyr::layer_name, std::make_unique<Lyr>(
				self.context(), self.batch.get_chunk_count(), self.get_public_sets(),
				std::forward<Args>(args)...)
			);

			if(!suc){
				throw std::invalid_argument{"Duplicated Layer Name"};
			}

			Lyr& rst = static_cast<Lyr&>(*itr->second);
			self.update_layer_commands(rst);
			return rst;
		}

		template <std::derived_from<batch_layer> Lyr>
		Lyr* find_named_batch_layer(std::string_view name) noexcept{
			if(const auto itr = layers_.find(name); itr != layers_.end()){
				return dynamic_cast<Lyr*>(itr->second.get());
			}
			return nullptr;
		}

		template <batch_effect_layer Lyr>
		Lyr& batch_layer_at() noexcept{
			return dynamic_cast<Lyr&>(*layers_.at(Lyr::layer_name));
		}

		batch_layer& get_current_batch_layer() = delete;

		template <std::derived_from<batch_proxy> S>
		batch_layer* set_batch_layer(this S& self, batch_layer* layer = nullptr) noexcept{
			return std::exchange(self.current_layer_, layer);
		}

		template <std::derived_from<batch_proxy> S>
		batch_layer* set_batch_layer(this S& self, batch_layer& layer) noexcept{
			return self.set_batch_layer(&layer);
		}

		vk::batch* operator->() noexcept{
			return &batch;
		}

		const vk::batch* operator->() const noexcept{
			return &batch;
		}

		[[nodiscard]] vk::context& context() const{
			assert(batch.get_context());
			return *batch.get_context();
		}

		operator vk::batch&() noexcept{
			return batch;
		}

		operator const vk::batch&() const noexcept{
			return batch;
		}
	};

	export
	template <std::derived_from<batch_proxy> Proxy, typename T>
	struct [[jetbrains::guard]] batch_layer_guard{
	private:
		Proxy& proxy;
		batch_layer* last_layer;

	public:
		[[nodiscard]] batch_layer_guard(Proxy& proxy, std::in_place_type_t<T>)
			noexcept : proxy{proxy}, last_layer{(proxy.batch.consume_all(), proxy.set_batch_layer(layer()))}
		{

		}

		T& layer() const noexcept{
			return proxy.template batch_layer_at<T>();
		}

		~batch_layer_guard(){
			proxy.batch.consume_all();
			proxy.set_batch_layer(last_layer);
		}

		batch_layer_guard(const batch_layer_guard& other) = delete;
		batch_layer_guard(batch_layer_guard&& other) noexcept = delete;
		batch_layer_guard& operator=(const batch_layer_guard& other) = delete;
		batch_layer_guard& operator=(batch_layer_guard&& other) noexcept = delete;
	};
}
