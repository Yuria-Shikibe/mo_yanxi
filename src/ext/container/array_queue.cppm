export module ext.array_queue;

import std;

export namespace mo_yanxi {
    template <typename T, std::size_t capacity>
        requires (std::is_default_constructible_v<T>)
    class array_queue {
    public:
        static constexpr std::size_t max_size{capacity};

        [[nodiscard]] constexpr array_queue() = default;

        consteval void push_and_replace(const T& element) noexcept(!DEBUG_CHECK && std::is_nothrow_copy_assignable_v<T>){
            if(full()){
                pop_front();
            }

            this->push(element);
        }

        consteval void push_and_replace(T&& element) noexcept(!DEBUG_CHECK && std::is_nothrow_move_assignable_v<T>){
            if(full()){
                pop_front();
            }

            this->push(std::move(element));
        }

        template <typename... Args>
        constexpr decltype(auto) emplace_and_replace(Args&& ...args) noexcept(!DEBUG_CHECK && noexcept(T{std::forward<Args>(args) ...})){
            if(full()){
                pop_front();
            }

            return this->emplace(std::forward<Args>(args) ...);
        }

        constexpr void push(const T &item) noexcept(!DEBUG_CHECK && std::is_nothrow_copy_assignable_v<T>) requires (std::is_copy_assignable_v<T>){
#if DEBUG_CHECK
            if(full()) {
                throw std::overflow_error("Queue is full");
            }
#endif

            data[backIndex] = item;
            backIndex = (backIndex + 1) % max_size;
            ++count;
        }

        constexpr void push(T&& item) noexcept(!DEBUG_CHECK && std::is_nothrow_move_assignable_v<T>) requires (std::is_move_assignable_v<T>){
#if DEBUG_CHECK
            if(full()) {
                throw std::overflow_error("Queue is full");
            }
#endif

            data[backIndex] = std::move(item);
            backIndex = (backIndex + 1) % max_size;
            ++count;
        }

        template <typename... Args>
            requires (std::constructible_from<T, Args...>)
        constexpr T& emplace(Args&& ...args) noexcept(!DEBUG_CHECK && noexcept(T{std::forward<Args>(args) ...})) requires (std::is_move_assignable_v<T>){
#if DEBUG_CHECK
            if(full()) {
                throw std::overflow_error("Queue is full");
            }
#endif

            T& rst = (data[backIndex] = T{std::forward<Args>(args) ...});
            backIndex = (backIndex + 1) % max_size;
            ++count;

            return rst;
        }

        constexpr void pop_front() {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }

            frontIndex = (frontIndex + 1) % max_size;
            --count;
        }

        constexpr void pop_back() {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }

            backIndex = backIndex == 0 ? max_size - 1 : backIndex - 1;
            --count;
        }

        constexpr std::optional<T> pop_front_and_get_opt() noexcept {
            if(empty()) {
                return std::nullopt;
            }else{
                std::optional<T> rst{std::move(front())};
                frontIndex = (frontIndex + 1) % max_size;
                --count;
                return rst;
            }
        }

        constexpr T pop_front_and_get() {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }else{
                T rst{std::move(front())};
                frontIndex = (frontIndex + 1) % max_size;
                --count;
                return rst;
            }
        }

        constexpr T pop_back_and_get() {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }else{
                T rst{std::move(back())};
                backIndex = (backIndex == 0 ? max_size : backIndex) - 1;
                --count;
                return rst;
            }
        }

        [[nodiscard]] constexpr decltype(auto) front() const {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }

            return data[frontIndex];
        }

        [[nodiscard]] constexpr decltype(auto) front() {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }

            return data[frontIndex];
        }

        [[nodiscard]] constexpr decltype(auto) back() const {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }

            return data[(backIndex - 1 + max_size) % max_size];
        }

        [[nodiscard]] constexpr decltype(auto) back() {
            if(empty()) {
                throw std::underflow_error("Queue is empty");
            }

            return data[(backIndex - 1 + max_size) % max_size];
        }

        [[nodiscard]] constexpr std::size_t front_index() const noexcept {
            return frontIndex;
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return count == 0;
        }

        [[nodiscard]] constexpr bool full() const noexcept {
            return count == max_size;
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return count;
        }

        T& operator[](const std::size_t index) noexcept{
            return data[index];
        }

        const T& operator[](const std::size_t index) const noexcept{
            return data[index];
        }

    private:
        std::array<T, capacity> data{};

        std::size_t frontIndex{};
        std::size_t backIndex{};
        std::size_t count{};
    };
}
