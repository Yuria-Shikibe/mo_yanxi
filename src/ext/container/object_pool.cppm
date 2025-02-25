module;

#include <vcruntime_typeinfo.h>

#include "../adapted_attributes.hpp"

export module ext.object_pool;

import ext.array_stack;
import ext.atomic_caller;
import std;

namespace mo_yanxi{

	template <typename T, std::size_t count>
	struct object_pool_page{
		T* data_uninitialized;
		mutable std::mutex mutex{};

		array_stack<T*, count> valid_pointers{};

		[[nodiscard]] object_pool_page() = default;

		[[nodiscard]] constexpr explicit object_pool_page(T* data) : data_uninitialized{data}{
			std::scoped_lock lk{mutex};

			for(std::size_t i = 0; i < count; ++i){
				valid_pointers.push(data_uninitialized + i);
			}
		}

		/** @brief DEBUG USAGE */
		[[nodiscard]] constexpr std::span<T> as_span() const noexcept{
			return std::span{data_uninitialized, count};
		}

		template <std::invocable<object_pool_page&> Func>
		FORCE_INLINE decltype(auto) lock_and(Func func){
			std::scoped_lock lk{mutex};
			return func(*this);
		}

		template<typename Alloc>
		FORCE_INLINE constexpr void free(Alloc& alloc) const noexcept{
			std::scoped_lock lk{mutex};

// #if DEBUG_CHECK
			if(!valid_pointers.full()){
				std::cerr << std::format("pool<{}> leaked: {}/{}\n", typeid(T).name(), valid_pointers.size(), count);
				// throw std::runtime_error("pool_page::free: not all pointers are stored");
			}
// #endif

			std::allocator_traits<Alloc>::deallocate(alloc, data_uninitialized, count);
		}

		FORCE_INLINE constexpr void store(T* p) noexcept(std::is_nothrow_destructible_v<T>){
			if constexpr(!std::is_trivially_destructible_v<T>){
				std::destroy_at(p);
			}

			std::scoped_lock lk{mutex};
			assert(!valid_pointers.full());
			valid_pointers.push(p);
		}

		/**
		 * @return nullptr if the page is empty, or uninitialized pointer
		 */
		constexpr T* borrow_uninitialized() noexcept{
			T* p;

			{
				std::scoped_lock lk{mutex};

				if(valid_pointers.empty()){
					return nullptr;
				}

				p = valid_pointers.top();
				valid_pointers.pop();
			}

			return p;
		}

		//warning : the resource manage is tackled by external call

		object_pool_page(const object_pool_page& other) = delete;

		constexpr object_pool_page(object_pool_page&& other) noexcept = delete;

		object_pool_page& operator=(const object_pool_page& other) = delete;

		constexpr object_pool_page& operator=(object_pool_page&& other) noexcept = delete;
	};

	template <typename T, std::size_t count>
	struct pool_deleter{
		object_pool_page<T, count>* page{};

		[[nodiscard]] pool_deleter() = default;

		[[nodiscard]] explicit pool_deleter(object_pool_page<T, count>* page)
			: page{page}{}

		void operator()(T* ptr) const noexcept{
			if(ptr)page->store(ptr);
		}
	};

	export
	/**
	 * @brief a really simple object pool
	 * @tparam T type
	 * @tparam PageSize maximum objects a page can contain
	 * @tparam Alloc allocator type ONLY SUPPORTS DEFAULT ALLOCATOR NOW
	 */
	template <typename T, std::size_t PageSize = 1000, typename AllocDummy = void>
	struct object_pool{
		using value_type = T;

		using page_type = object_pool_page<T, PageSize>;
		using deleter_type = pool_deleter<T, PageSize>;
		using unique_ptr = std::unique_ptr<T, deleter_type>;
		using allocator_type = std::allocator<T>;

		struct obtain_return_type{
			value_type* data;
			page_type* page;
		};

	private:
		// mutable std::mutex mutex{};
		atomic_caller<page_type*> appendPageCaller{};
		// std::condition_variable condition{};

		std::list<page_type> pages{};

		ADAPTED_NO_UNIQUE_ADDRESS
		allocator_type allocator{};

	public:
		template <typename... Args>
			requires (std::constructible_from<T, Args...>)
		[[nodiscard]] unique_ptr obtain_unique(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>){
			auto [ptr, page] = this->obtain_raw(std::forward<Args>(args)...);
			return unique_ptr{ptr, deleter_type{page}};
		}

		template <typename... Args>
			requires (std::constructible_from<T, Args...>)
		[[nodiscard]] std::shared_ptr<T> obtain_shared(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>){
			auto [ptr, page] = this->template obtain_raw<Args...>(std::forward<Args>(args)...);
			return std::shared_ptr<T>{ptr, deleter_type{page}};
		}

		template <typename... Args>
			requires (std::constructible_from<T, Args...>)
		[[nodiscard]] obtain_return_type obtain_raw(Args&&... args)
			noexcept(std::is_nothrow_constructible_v<T, Args...>){
			obtain_return_type rstPair{};

			appendPageCaller.wait_if_is_exec();

			for(page_type& page : pages){
				rstPair.data = page.borrow_uninitialized();
				if(rstPair.data){
					rstPair.page = &page;
					goto ret;
				}
			}

			while(rstPair.data == nullptr){
				rstPair.page = appendPageCaller.exec([this]() noexcept{
					T* ptr = std::allocator_traits<allocator_type>::allocate(allocator, PageSize);
					page_type& page = pages.emplace_back(ptr);
					return &page;
				});
				rstPair.data = rstPair.page->borrow_uninitialized();
			}

			ret:

			rstPair.data = std::construct_at(rstPair.data, std::forward<Args>(args)...);

			return rstPair;
		}

		/** @brief DEBUG USAGE */
		[[nodiscard]] const std::list<page_type>& get_pages() const noexcept{
			return pages;
		}

		[[nodiscard]] object_pool() = default;

		~object_pool(){
			for (const auto & page : pages){
				page.free(allocator);
			}
		}

		object_pool(const object_pool& other) = delete;

		object_pool(object_pool&& other) noexcept : pages(std::exchange(other.pages, {})){
		}

		object_pool& operator=(const object_pool& other) = delete;

		//TODO correct move for allocators
		object_pool& operator=(object_pool&& other) noexcept{
			if(this == &other) return *this;
			// std::swap(allocator, other.allocator);
			std::swap(pages, other.pages);
			return *this;
		}
	};
}
