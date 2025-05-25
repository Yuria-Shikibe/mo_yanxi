
export module mo_yanxi.game.ecs.component.manage:serializer;

import std;

#if DEBUG_CHECK
#define CHECKED_STATIC_CAST(type) dynamic_cast<type>
#else
#define CHECKED_STATIC_CAST(type) static_cast<type>
#endif


namespace mo_yanxi::game::ecs{
	export struct archetype_base;
	export struct component_manager;

	export using component_chunk_offset = std::uint64_t;

	constexpr inline auto endian_need_swap = std::endian::big;

	constexpr inline void swapbyte_if_needed(std::integral auto& val){
		val = std::byteswap(val);
	}

	export struct archetype_serialize_identity{
		static constexpr std::uint64_t unknown{};
		std::uint64_t index{unknown};
	};

	export struct bad_archetype_serialize : std::runtime_error{
		using std::runtime_error::runtime_error;
	};

	export struct archetype_serializer{
		virtual ~archetype_serializer() = default;

		[[nodiscard]] virtual archetype_serialize_identity get_identity() const noexcept{
			return {};
		}
		// virtual void dump(archetype_base& base) = 0;
		virtual void write(std::ostream& stream) const = 0;
		virtual void read(std::istream& stream) = 0;
		virtual void load(component_manager& target) const = 0;
		virtual void clear() noexcept = 0;
	};


	struct await_offset_tag_t{};
	constexpr inline await_offset_tag_t await_offset_tag{};

	export enum struct srl_state{
		unfinished,
		failed,
		succeed
	};

	export
	struct chunk_serialize_handle{
		struct promise_type;
		using handle = std::coroutine_handle<promise_type>;

		[[nodiscard]] chunk_serialize_handle() noexcept = default;

		[[nodiscard]] explicit chunk_serialize_handle(handle&& hdl)
			: hdl{std::move(hdl)}{}

		struct promise_type{
			std::uint32_t chunk_offset{};
			srl_state state{};

			[[nodiscard]] promise_type() = default;

			chunk_serialize_handle get_return_object() noexcept{
				return chunk_serialize_handle{handle::from_promise(*this)};
			}

			[[nodiscard]] static auto initial_suspend() noexcept{ return std::suspend_never{}; }

			[[nodiscard]] static auto final_suspend() noexcept{ return std::suspend_always{}; }

			std::suspend_always yield_value(std::uint32_t off) noexcept{
				if(off == 0){
					state = srl_state::failed;
				}
				chunk_offset = off;
				return {};
			}

			void return_void() noexcept{
				state = srl_state::succeed;
			}

			[[noreturn]] void unhandled_exception() noexcept{
				state = srl_state::failed;
			}


		private:
			friend chunk_serialize_handle;

		};

		void resume() const{
			hdl.resume();
		}

		[[nodiscard]] bool done() const noexcept{
			return hdl.done();
		}

		chunk_serialize_handle(const chunk_serialize_handle& other) = delete;

		chunk_serialize_handle(chunk_serialize_handle&& other) noexcept
			: hdl{std::exchange(other.hdl, {})}{
		}

		chunk_serialize_handle& operator=(const chunk_serialize_handle& other) = delete;

		chunk_serialize_handle& operator=(chunk_serialize_handle&& other) noexcept{
			if(this == &other) return *this;
			dstry();
			hdl = std::exchange(other.hdl, {});
			return *this;
		}

		~chunk_serialize_handle(){
			dstry();
		}

		auto get_state() const noexcept{
			return hdl.promise().state;
		}

		[[nodiscard]] std::uint32_t get_offset() const{
			if(get_state() == srl_state::failed){
				throw std::runtime_error("chunk_serialize_handle::get_offset() failed");
			}

			return hdl.promise().chunk_offset;
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
	struct component_pack{
	private:
		std::vector<std::unique_ptr<archetype_serializer>> chunks;

	public:
		[[nodiscard]] component_pack() = default;

		[[nodiscard]] explicit(false) component_pack(const component_manager& manager);

		void clear_items() const noexcept{
			for (auto& archetype_serializer : chunks){
				archetype_serializer->clear();
			}
		}
		void clear() noexcept{
			chunks.clear();
		}

		void write(std::ostream& stream) const;

		void read(std::istream& stream);

		void load_to(component_manager& manager) const;
	};

}
