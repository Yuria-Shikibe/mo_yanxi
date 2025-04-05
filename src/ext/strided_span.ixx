module;

#include <cassert>

export module mo_yanxi.strided_span;

import std;

namespace mo_yanxi{
	template <class It, class T>
	concept Span_compatible_iterator =
		std::contiguous_iterator<It> && std::is_convertible_v<std::remove_reference_t<std::iter_reference_t<It>> (*)[], T (*)[]>;

	template <class Se, class It>
	concept Span_compatible_sentinel = std::sized_sentinel_for<Se, It> && !std::is_convertible_v<Se, size_t>;

	export
	enum span_stride : std::ptrdiff_t{
		dynamic_in_byte = 0//std::numeric_limits<std::ptrdiff_t>::max(),
	};

	template <typename T, T size>
	struct optional_dynamic_size{
		static constexpr T value = size;

		[[nodiscard]] optional_dynamic_size() = default;
		[[nodiscard]] optional_dynamic_size(T t){}

		constexpr explicit(false) operator T() const noexcept{
			return value;
		}
	};

	template <>
	struct optional_dynamic_size<std::size_t, std::dynamic_extent>{
		std::size_t value{};

		constexpr explicit(false) operator std::size_t() const noexcept{
			return value;
		}
	};

	template <>
	struct optional_dynamic_size<std::ptrdiff_t, dynamic_in_byte>{
		std::ptrdiff_t value{};

		constexpr explicit(false) operator std::ptrdiff_t() const noexcept{
			return value;
		}
	};

	export
	template <class Ty, std::ptrdiff_t Stride = dynamic_in_byte>
	struct strided_span_iterator {
		using iterator_concept  = std::contiguous_iterator_tag;
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = std::remove_cv_t<Ty>;
		using difference_type   = std::ptrdiff_t;
		using pointer           = Ty*;
		using reference         = Ty&;

		[[nodiscard]] constexpr reference operator*() const noexcept {
			assert(ptr != nullptr);
			return *ptr;
		}

		[[nodiscard]] constexpr pointer operator->() const noexcept {
			return ptr;
		}

		constexpr strided_span_iterator& operator++() noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) + stride.value);
			}else{
				++ptr;
			}

			return *this;
		}

		constexpr strided_span_iterator operator++(int) noexcept {
			strided_span_iterator tmp{*this};
			++*this;
			return tmp;
		}

		constexpr strided_span_iterator& operator--() noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) - stride.value);
			}else{
				--ptr;
			}
			return *this;
		}

		constexpr strided_span_iterator operator--(int) noexcept {
			strided_span_iterator tmp{*this};
			--*this;
			return tmp;
		}

		constexpr strided_span_iterator& operator+=(const difference_type off) noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) + off * stride.value);
			}else{
				ptr += off;
			}

			return *this;
		}

		[[nodiscard]] constexpr strided_span_iterator operator+(const difference_type Off) const noexcept {
			strided_span_iterator Tmp{*this};
			Tmp += Off;
			return Tmp;
		}

		[[nodiscard]] friend constexpr strided_span_iterator operator+(const difference_type Off, strided_span_iterator Next) noexcept {
			Next += Off;
			return Next;
		}

		constexpr strided_span_iterator& operator-=(const difference_type off) noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) - off * stride.value);
			}else{
				ptr -= off;
			}

			return *this;
		}

		[[nodiscard]] constexpr strided_span_iterator operator-(const difference_type off) const noexcept {
			strided_span_iterator tmp{*this};
			tmp -= off;
			return tmp;
		}

		[[nodiscard]] constexpr difference_type operator-(const strided_span_iterator& right) const noexcept {
			return ptr - right.ptr;
		}

		[[nodiscard]] constexpr reference operator[](const difference_type off) const noexcept {
			return *(*this + off);
		}

		[[nodiscard]] constexpr bool operator==(const strided_span_iterator& right) const noexcept {
			return ptr == right.ptr;
		}

		[[nodiscard]] constexpr auto operator<=>(const strided_span_iterator& right) const noexcept {
			return ptr <=> right.ptr;
		}

		[[nodiscard]] strided_span_iterator() = default;

		[[nodiscard]] explicit(false) strided_span_iterator(pointer ptr, std::ptrdiff_t stride = 0)
			: ptr(ptr),
			  stride(stride){
		}

	private:
		pointer ptr = nullptr;
		optional_dynamic_size<difference_type, Stride> stride{};

	};

	export
	template <class Ty, std::size_t Extent = std::dynamic_extent, std::ptrdiff_t Stride = dynamic_in_byte>
	class strided_span{
	private:
		Ty* ptr_;
		optional_dynamic_size<std::size_t, Extent> extent_;
		optional_dynamic_size<std::ptrdiff_t, Stride> stride_;

	public:
		using element_type = Ty;
		using value_type = std::remove_cv_t<Ty>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using const_pointer = const Ty*;
		using reference = Ty&;
		using const_reference = const Ty&;
		using iterator = strided_span_iterator<Ty>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_iterator         = std::const_iterator<iterator>;
		using const_reverse_iterator = std::const_iterator<reverse_iterator>;


		template <class Ty_, std::size_t Extent_, std::ptrdiff_t Stride_>
		friend class strided_span;
		// static constexpr size_type extent = Extent;

		// [span.cons] Constructors, copy, and assignment
		constexpr strided_span() noexcept requires (Extent == 0 || Extent == std::dynamic_extent)
		= default;

		template <Span_compatible_iterator<element_type> It>
		constexpr explicit(Extent != std::dynamic_extent) strided_span(It first, size_type count, difference_type stride = 0) noexcept // strengthened
			: ptr_(std::to_address(first)), extent_(count), stride_(stride){
		}


		template <Span_compatible_iterator<element_type> It, Span_compatible_sentinel<It> Se>
		constexpr explicit(Extent != std::dynamic_extent) strided_span(It first, Se last, difference_type stride = 0)
			noexcept(noexcept(last - first)) // strengthened
			: ptr_(std::to_address(first)), extent_(static_cast<size_type>(last - first)), stride_(stride){
		}

		template <typename T>
		constexpr explicit strided_span(strided_span<T, Extent, Stride> other)
			: ptr_(reinterpret_cast<Ty*>(other.data())), extent_(other.extent_.value * sizeof(T) / sizeof(value_type)), stride_(other.stride_.value){
		}


		template <std::size_t Sz>
			requires (Extent == std::dynamic_extent || Extent == Sz)
		constexpr strided_span(std::type_identity_t<element_type> (&arr)[Sz], difference_type stride = 0) noexcept
			: ptr_(arr), extent_(Sz), stride_(stride){
		}

		strided_span(const strided_span& other) = default;
		strided_span(strided_span&& other) noexcept = default;
		strided_span& operator=(const strided_span& other) = default;
		strided_span& operator=(strided_span&& other) noexcept = default;

		/*
		template <class _OtherTy, size_t _Size>
			requires (Extent == std::dynamic_extent || Extent == _Size) && is_convertible_v<
				_OtherTy (*)[], element_type (*)[]>
		constexpr span(array<_OtherTy, _Size>& _Arr) noexcept : _Mybase(_Arr.data(), _Size){
		}

		template <class _OtherTy, size_t _Size>
			requires (Extent == std::dynamic_extent || Extent == _Size)
			&& is_convertible_v<const _OtherTy (*)[], element_type (*)[]>
		constexpr span(const array<_OtherTy, _Size>& _Arr) noexcept : _Mybase(_Arr.data(), _Size){
		}

		template <_Span_compatible_range<element_type> _Rng>
		constexpr explicit(Extent != std::dynamic_extent) span(_Rng&& _Range)
			: _Mybase(_RANGES data(_Range), static_cast<size_type>(_RANGES size(_Range))) {
#if _CONTAINER_DEBUG_LEVEL > 0
			if constexpr (_Extent != std::dynamic_extent) {
				_STL_VERIFY(_RANGES size(_Range) == _Extent,
					"Cannot construct span with static extent from range r as std::ranges::size(r) != extent");
			}
#endif // _CONTAINER_DEBUG_LEVEL > 0
		}

		template <class _OtherTy, size_t _OtherExtent>
			requires (Extent == std::dynamic_extent || _OtherExtent == std::dynamic_extent || Extent == _OtherExtent)
			&& is_convertible_v<_OtherTy (*)[], element_type (*)[]>
		constexpr explicit(Extent != std::dynamic_extent && _OtherExtent == std::dynamic_extent)
		span(const span<_OtherTy, _OtherExtent>& _Other) noexcept
			: _Mybase(_Other.data(), _Other.size()){
#if _CONTAINER_DEBUG_LEVEL > 0
			if constexpr (_Extent != std::dynamic_extent) {
				_STL_VERIFY(_Other.size() == _Extent,
					"Cannot construct span with static extent from other span as other.size() != extent");
			}
#endif // _CONTAINER_DEBUG_LEVEL > 0
		}

		// [span.sub] Subviews
		template <size_t _Count>
	[[nodiscard]] constexpr auto first() const noexcept /* strengthened #1#{
			if constexpr(Extent != std::dynamic_extent){
				static_assert(_Count <= Extent, "Count out of range in span::first()");
			}
#if _CONTAINER_DEBUG_LEVEL > 0
			else {
				_STL_VERIFY(_Count <= _Mysize, "Count out of range in span::first()");
			}
#endif // _CONTAINER_DEBUG_LEVEL > 0
			return span<element_type, _Count>{_Mydata, _Count};
		}

		[[nodiscard]] constexpr auto first(const size_type _Count) const noexcept
			/* strengthened #1#{
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(_Count <= _Mysize, "Count out of range in span::first(count)");
#endif // _CONTAINER_DEBUG_LEVEL > 0
			return span<element_type, std::dynamic_extent>{_Mydata, _Count};
		}

		template <size_t _Count>
	[[nodiscard]] constexpr auto last() const noexcept /* strengthened #1#{
			if constexpr(Extent != std::dynamic_extent){
				static_assert(_Count <= Extent, "Count out of range in span::last()");
			}
#if _CONTAINER_DEBUG_LEVEL > 0
			else {
				_STL_VERIFY(_Count <= _Mysize, "Count out of range in span::last()");
			}
#endif // _CONTAINER_DEBUG_LEVEL > 0
			return span<element_type, _Count>{_Mydata + (_Mysize - _Count), _Count};
		}

		[[nodiscard]] constexpr auto last(const size_type _Count) const noexcept /* strengthened #1#{
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(_Count <= _Mysize, "Count out of range in span::last(count)");
#endif // _CONTAINER_DEBUG_LEVEL > 0
			return span<element_type, std::dynamic_extent>{_Mydata + (_Mysize - _Count), _Count};
		}

		template <size_t _Offset, size_t _Count = std::dynamic_extent>
	[[nodiscard]] constexpr auto subspan() const noexcept /* strengthened #1#{
			if constexpr(Extent != std::dynamic_extent){
				static_assert(_Offset <= Extent, "Offset out of range in span::subspan()");
				static_assert(
					_Count == std::dynamic_extent || _Count <= Extent - _Offset, "Count out of range in span::subspan()");
			}
#if _CONTAINER_DEBUG_LEVEL > 0
			else {
				_STL_VERIFY(_Offset <= _Mysize, "Offset out of range in span::subspan()");

				if constexpr (_Count != std::dynamic_extent) {
					_STL_VERIFY(_Count <= _Mysize - _Offset, "Count out of range in span::subspan()");
				}
			}
#endif // _CONTAINER_DEBUG_LEVEL > 0
			using _ReturnType = span<element_type,
									 _Count != std::dynamic_extent
										 ? _Count
										 : (Extent != std::dynamic_extent ? Extent - _Offset : std::dynamic_extent)>;
			return _ReturnType{_Mydata + _Offset, _Count == std::dynamic_extent ? _Mysize - _Offset : _Count};
		}

		[[nodiscard]] constexpr auto subspan(const size_type _Offset, const size_type _Count = std::dynamic_extent) const noexcept
			/* strengthened #1#{
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(_Offset <= _Mysize, "Offset out of range in span::subspan(offset, count)");
			_STL_VERIFY(_Count == std::dynamic_extent || _Count <= _Mysize - _Offset,
				"Count out of range in span::subspan(offset, count)");
#endif // _CONTAINER_DEBUG_LEVEL > 0
			using _ReturnType = span<element_type, std::dynamic_extent>;
			return _ReturnType{_Mydata + _Offset, _Count == std::dynamic_extent ? _Mysize - _Offset : _Count};
		}*/

		constexpr explicit(false) operator strided_span<std::byte, Extent, Stride>() noexcept{
			return {reinterpret_cast<std::byte*>(data()), extent_  * sizeof(value_type), stride_};
		}

		constexpr explicit(false) operator strided_span<const std::byte, Extent, Stride>() const noexcept {
			return {reinterpret_cast<const std::byte*>(data()), extent_  * sizeof(value_type), stride_};
		}
		// [span.obs] Observers
		[[nodiscard]] constexpr size_type size() const noexcept{
			return extent_.value;
		}

		[[nodiscard]] constexpr difference_type stride() const noexcept{
			return stride_.value;
		}

		[[nodiscard]] constexpr size_type size_bytes() const noexcept{
			return extent_.value * sizeof(element_type);
		}

		[[nodiscard]] constexpr bool empty() const noexcept{
			return extent_.value == 0;
		}

		// [span.elem] Element access
		[[nodiscard]] constexpr reference operator[](const size_type off) const noexcept /* strengthened */{
			assert(ptr_);
			assert(off < extent_.value);

			if(stride_.value){
				return ptr_[off];
			}else{
				const auto dst = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr_)) + off * stride_.value);
				return *dst;
			}
		}

		[[nodiscard]] constexpr reference front() const noexcept /* strengthened */{
			assert(ptr_);
			assert(extent_.value > 0);
			return *ptr_;
		}

		[[nodiscard]] constexpr reference back() const noexcept /* strengthened */{
			assert(ptr_);
			assert(extent_.value > 0);
			if(stride_.value){
				const auto end = ptr_ + extent_ - 1;
				return *end;
			}else{
				const auto end = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr_)) + (extent_ - 1) * stride_.value);
				return *end;
			}
		}

		[[nodiscard]] constexpr pointer data() const noexcept{
			return ptr_;
		}

		// [span.iterators] Iterator support
		[[nodiscard]] constexpr iterator begin() const noexcept{
			return iterator{ptr_, stride_.value};
		}

		[[nodiscard]] constexpr iterator end() const noexcept{
			if(stride_.value){
				const auto end = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr_)) + (extent_.value * stride_.value));
				return iterator{end, stride_.value};
			}else{
				const auto end = ptr_ + extent_.value;
				return iterator{end};
			}
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
			return begin();
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept {
			return end();
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept{
			return reverse_iterator{end()};
		}

		[[nodiscard]] constexpr reverse_iterator rend() const noexcept{
			return reverse_iterator{begin()};
		}

		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
			return rbegin();
		}

		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
			return rend();
		}
	};
}
