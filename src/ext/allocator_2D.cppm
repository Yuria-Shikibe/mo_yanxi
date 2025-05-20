module;

#include <cassert>

export module mo_yanxi.allocator_2D;

//TODO move geom vector2d/rect to ext
import mo_yanxi.math.vector2;
import mo_yanxi.math;
import mo_yanxi.math.rect_ortho;
import mo_yanxi.open_addr_hash_map;
import mo_yanxi.algo;
import std;

namespace mo_yanxi {
    using extent_size_t = unsigned;
    using internal_point_t = math::ushort_point2;

    struct size_chunked_vector{
        static constexpr extent_size_t chunk_split = 64;
        static constexpr extent_size_t minimum_chunk_size = 4;

        static constexpr std::size_t get_current_group_index(const extent_size_t size){
            if(size == 0)return 0;
            auto snap = std::bit_ceil(get_snap(size)) / chunk_split;

            std::size_t idx{0};

            while(snap){
                idx++;
                snap /= 2;
            }

            return idx - 1;
        }

        static constexpr extent_size_t index_to_size(const std::size_t index){
            return math::pow_integral(2, index) * chunk_split;
        }

        static constexpr extent_size_t get_snap(const extent_size_t size){
            return ((size - 1) / chunk_split + 1) * chunk_split;
        }

        static constexpr std::size_t get_chunk_capacity(const extent_size_t total_size, const extent_size_t size){
            const auto snap = get_snap(size);
            const auto index = get_current_group_index(size);

            if(size < 1024){
                const auto line = total_size / snap;
                const auto scale = math::curve<double>(static_cast<double>(index) + .75, -2, 5) * .5f;

                return
                    static_cast<std::size_t>(
                        static_cast<double>(line * line) * scale +
                        math::clamp<double>(static_cast<double>(total_size / snap) * 8, 0, 2048)
                    );
            }else{
                const auto line = total_size / snap;
                return line * line / 2;
            }
        }

        static constexpr std::size_t get_prefix_index(const extent_size_t total_size, const extent_size_t size) noexcept{
            auto index = get_current_group_index(size);

            std::size_t sz{};
            std::size_t exp{1};
            for(std::size_t i = 0; i < index; ++i){
                sz += get_chunk_capacity(total_size, exp * chunk_split);
                exp *= 2;
            }
            return sz;
        }


        struct slot{
            extent_size_t size;
            math::ushort_point2 identity;

            constexpr auto operator<=>(const slot&) const noexcept = default;
        };

        using value_type = slot;
        using container_type = std::vector<value_type>;


        struct acquire_rst{
            std::size_t index;
            std::ranges::subrange<container_type::iterator> candidates;
        };

        static constexpr std::size_t maximum_index = 12;

    private:
        std::array<container_type::difference_type, maximum_index> prefix{};
        std::array<container_type::difference_type, maximum_index> sizes_{};
        std::size_t ceil{};

        container_type vector{};

        constexpr void partial_insert(std::size_t index, container_type::iterator where, container_type::iterator end, const slot value) noexcept{
            sizes_[index]++;
            std::ranges::move_backward(where, end, end + 1);
            *where = value;
        }

        constexpr void partial_erase(std::size_t index, container_type::iterator where, container_type::iterator end) noexcept{
            sizes_[index]--;
            std::ranges::move(where + 1, end, where);
        }

    public:
        [[nodiscard]] constexpr size_chunked_vector() = default;

        [[nodiscard]] constexpr explicit size_chunked_vector(const extent_size_t total_size){
            ceil = get_current_group_index(total_size);

            std::size_t i = 0;
            for(; i < math::min(ceil, prefix.size() - 1); ++i){
                prefix[i + 1] = prefix[i] + static_cast<container_type::difference_type>(get_chunk_capacity(total_size, index_to_size(i))) + i * i * 2;
            }

            vector.resize(prefix[ceil] + ceil * ceil * 2);
        }

        constexpr std::span<value_type> vector_at(const extent_size_t size) noexcept{
            const auto index = math::min(ceil, get_current_group_index(size));
            const auto heap_begin = vector.begin() + prefix[index];
            const auto heap_end = heap_begin + sizes_[index];
            return std::span{heap_begin, heap_end};
        }

        [[nodiscard]] constexpr acquire_rst acquire_pos(const extent_size_t size, const std::size_t srcIndex = 0) noexcept{
            if(srcIndex > ceil){
                return {maximum_index};
            }

            auto index = math::clamp(get_current_group_index(size), srcIndex, ceil);

            auto begin = vector.begin() + prefix[index];
            auto end = begin + sizes_[index];

            auto find = std::ranges::upper_bound(begin, end, size, std::ranges::less_equal{}, &slot::size);

            if(find == end){
                ++index;
                while(index != ceil + 1){
                    begin = vector.begin() + prefix[index];
                    end = begin + sizes_[index];

                    if(begin != end){
                        return {index, {begin, end}};
                    }else{
                        ++index;
                    }
                }

                return {maximum_index};
            }

            return {index, {find, end}};
        }

        constexpr bool insert_or_assign(const extent_size_t size, const math::upoint2 where) noexcept{
            auto raw_idx = get_current_group_index(size);
            const auto index = math::min(ceil, raw_idx);
            const auto begin = vector.begin() + prefix[index];
            const auto end = begin + sizes_[index];
            const auto sentinel = raw_idx == ceil ? vector.end() : vector.begin() + prefix[index + 1];

            if(end != sentinel){
                slot s{
                    .size = size,
                    .identity = where.as<internal_point_t>()
                };

                const auto b = std::ranges::lower_bound(begin, end, s);
                if(b != end && b->identity == s.identity){
                    *b = s;
                    return true;
                }

                partial_insert(index, b, end, s);
                return true;
            }

            return false;
        }

        constexpr bool erase(const extent_size_t size, const math::upoint2 where) noexcept{
            const auto index = get_current_group_index(size);

            const auto begin = vector.begin() + prefix[index];
            const auto end = begin + sizes_[index];

            slot s{
                .size = size,
                .identity = where.as<internal_point_t>()
            };

            auto pos = std::ranges::lower_bound(begin, end, s);
            if(pos != end && pos->identity == where.as<internal_point_t>()){
                partial_erase(index, pos, end);
                return true;
            }

            return false;
        }
    };

    export
    struct allocator2d{
    protected:
        using T = std::uint32_t;

    public:
        using size_type = T;
        using extent_type = math::vector2<T>;
        using point_type = math::vector2<T>;
        using rect_type = math::rect_ortho<T>;
        using sub_region_type = std::array<rect_type, 3>;

    protected:
        extent_type extent_{};
        size_type remain_area_{};

        struct point_to_size{
            internal_point_t pos;
            extent_size_t size;

            constexpr auto operator<=>(const point_to_size&) const noexcept = default;
        };

        struct split_point;
        using map_type =
            std::unordered_map<point_type, split_point>;
            // fixed_open_hash_map<point_type, split_point, math::vectors::constant2<point_type::value_type>::max_vec2>;

        map_type map{};

        using tree_type = std::map<T, std::multimap<T, point_type>>;

        tree_type nodes_XY{};
        tree_type nodes_YX{};

        using ItrOuter = tree_type::iterator;
        using ItrInner = tree_type::mapped_type::iterator;

        struct ItrPair{
            ItrOuter outer{};
            ItrInner inner{};

            [[nodiscard]] auto value() const{
                return inner->second;
            }

            /**
             * @return [rst, possible to find]
             */
            void locateNextInner(const T second){

                inner = outer->second.lower_bound(second);
            }

            bool locateNextOuter(tree_type& tree){
                ++outer;
                if(outer == tree.end()) return false;
                return true;
            }

            [[nodiscard]] bool valid(const tree_type& tree) const noexcept{
                return outer != tree.end() && inner != outer->second.end();
            }
        };

        struct split_point{
            point_type parent{};

            point_type bot_lft{};
            point_type top_rit{};

            point_type split{top_rit};

            bool idle{true};
            bool idle_top_lft{true};
            bool idle_top_rit{true};
            bool idle_bot_rit{true};


            [[nodiscard]] split_point() = default;

            [[nodiscard]] split_point(
                const point_type parent,
                const point_type bot_lft,
                const point_type top_rit)
                : parent(parent),
                  bot_lft(bot_lft), top_rit(top_rit){
            }

            [[nodiscard]] bool is_leaf() const noexcept{
                return split == top_rit;
            }

            [[nodiscard]] bool is_root() const noexcept{
                return parent == bot_lft;
            }

            [[nodiscard]] bool is_split_idle() const noexcept{
                return idle_top_lft && idle_top_rit && idle_bot_rit;
            }

            void mark_captured(map_type& map) noexcept{
                idle = false;

                if(is_root())return;
                auto& p = get_parent(map);
                if(parent.x == bot_lft.x){
                    p.idle_top_lft = false;
                }else if(parent.y == bot_lft.y){
                    p.idle_bot_rit = false;
                }else{
                    p.idle_top_rit = false;
                }
            }

            bool check_merge(allocator2d& alloc) noexcept{
                if(idle && is_split_idle()){
                    //resume split
                    if(region_top_lft().area())alloc.erase_split(src_top_lft(), {split.x, top_rit.y});
                    if(region_top_rit().area())alloc.erase_split(src_top_rit(), top_rit);
                    if(region_bot_rit().area())alloc.erase_split(src_bot_rit(), {top_rit.x, split.y});
                    alloc.erase_mark(bot_lft, split);

                    split = top_rit;

                    if(is_root())return false;

                    auto& p = get_parent(alloc.map);
                    if(parent.x == bot_lft.x){
                        p.idle_top_lft = true;
                    }else if(parent.y == bot_lft.y){
                        p.idle_bot_rit = true;
                    }else{
                        p.idle_top_rit = true;
                    }

                    return true;
                }

                return false;
            }

            [[nodiscard]] extent_type get_extent() const noexcept{
                return split - bot_lft;
            }

            split_point& get_parent(map_type& map) const noexcept{
                return map.at(parent);
            }

            [[nodiscard]] constexpr point_type src_top_lft() const noexcept{
                return {bot_lft.x, split.y};
            }

            [[nodiscard]] constexpr point_type src_bot_rit() const noexcept{
                return {split.x, bot_lft.y};
            }

            [[nodiscard]] constexpr point_type src_top_rit() const noexcept{
                return split;
            }

            [[nodiscard]] constexpr rect_type region_bot_lft() const noexcept{
                return {tags::unchecked, bot_lft, split};
            }

            [[nodiscard]] constexpr rect_type region_bot_rit() const noexcept{
                return {tags::unchecked, {split.x, bot_lft.y}, {top_rit.x, split.y}};
            }

            [[nodiscard]] constexpr rect_type region_top_rit() const noexcept{
                return {tags::unchecked, split, top_rit};
            }

            [[nodiscard]] constexpr rect_type region_top_lft() const noexcept{
                return {tags::unchecked, {bot_lft.x, split.y}, {split.x, top_rit.y}};
            }

            void acquire_and_split(allocator2d& alloc, const math::usize2 extent){
                assert(idle);

                if(is_leaf()){
                    // first split
                    split = bot_lft + extent;
                    alloc.erase_mark(bot_lft, top_rit);

                    if(const auto b_r = region_bot_rit(); b_r.area()){
                        alloc.add_split(bot_lft, b_r.get_src(), b_r.get_end());
                    }

                    if(const auto t_r = region_top_rit(); t_r.area()){
                        alloc.add_split(bot_lft, t_r.get_src(), t_r.get_end());
                    }

                    if(const auto t_l = region_top_lft(); t_l.area()){
                        alloc.add_split(bot_lft, t_l.get_src(), t_l.get_end());
                    }

                    mark_captured(alloc.map);
                }else{
                    alloc.erase_mark(bot_lft, split);
                }
            }

            split_point* mark_idle(allocator2d& alloc){
                if(idle)return nullptr;
                idle = true;

                auto* p = this;
                while(p->check_merge(alloc)){
                    auto* next = &p->get_parent(alloc.map);
                    p = next;
                }

                if(p->is_leaf())alloc.mark_size(p->bot_lft, p->top_rit);
                return p;
            }
        };

    private:
        std::optional<point_type> getValidNode(const extent_type size){
            assert(size.area() > 0);

            if(remain_area_ < size.area()){
                return std::nullopt;
            }

            ItrPair itrPairXY{nodes_XY.lower_bound(size.x)};
            ItrPair itrPairYX{nodes_YX.lower_bound(size.y)};

            std::optional<point_type> node{};

            bool possibleX{itrPairXY.outer != nodes_XY.end()};
            bool possibleY{itrPairYX.outer != nodes_YX.end()};

            while(true)
            {
                if(!possibleX && !possibleY)break;

                if(possibleX){
                    itrPairXY.locateNextInner(size.y);
                    if (itrPairXY.valid(nodes_XY)){
                        node = itrPairXY.value();
                        break;
                    }else{
                        possibleX = itrPairXY.locateNextOuter(nodes_XY);
                    }
                }

                if(possibleY){
                    itrPairYX.locateNextInner(size.x);
                    if (itrPairYX.valid(nodes_YX)){
                        node = itrPairYX.value();
                        break;
                    }else{
                        possibleY = itrPairYX.locateNextOuter(nodes_YX);
                    }
                }
            }

            return node;
        }

        void mark_size(const point_type src, const point_type dst) noexcept{
            const auto size = dst - src;

            nodes_XY[size.x].insert({size.y, src});
            nodes_YX[size.y].insert({size.x, src});
        }

        void add_split(const point_type parent, const point_type src, const point_type dst){
            map.insert_or_assign(src, split_point{parent, src, dst});
            mark_size(src, dst);
        }

        void erase_split(const point_type src, const point_type dst){
            map.erase(src);
            erase_mark(src, dst);
        }

        void erase_mark(const point_type src, const point_type dst){
            const auto size = dst - src;

            erase(nodes_XY, src, size.x, size.y);
            erase(nodes_YX, src, size.y, size.x);
        }

        static void erase(tree_type& map, const point_type src, const size_type outerKey, const size_type innerKey) noexcept {
            auto& inner = map[outerKey];
            auto [begin, end] = inner.equal_range(innerKey);//->second.erase(inner);
            for(auto cur = begin; cur != end; ++cur){
                if(cur->second == src){
                    inner.erase(cur);
                    return;
                }
            }
        }

    public:
        [[nodiscard]] allocator2d() = default;

        [[nodiscard]] explicit(false) allocator2d(const extent_type extent) :
        extent_(extent), remain_area_(extent.area()){
            map.reserve(1024);
            add_split({}, {}, extent);
        }

        std::optional<point_type> allocate(const math::usize2 extent) noexcept{
            if(extent.area() == 0){
                std::println(std::cerr, "try allocate region with zero area");
                std::terminate();
            }

            if(extent.beyond(extent_))return std::nullopt;
            if(remain_area_ < extent.as<std::uint64_t>().area())return std::nullopt;

            auto chamber_src = getValidNode(extent);

            if(!chamber_src)return std::nullopt;

            auto& chamber = map[chamber_src.value()];
            chamber.acquire_and_split(*this, extent);
            remain_area_ -= extent.area();
            return chamber_src.value();
        }

        void deallocate(const point_type value) noexcept{
            if(const auto itr = map.find(value); itr != map.end()){
                if(const auto chamber = itr->second.mark_idle(*this)){
                    const auto extent = chamber->get_extent().area();
                    remain_area_ += extent;
                }
            }else{
                std::println(std::cerr, "try deallocate region not belong to the allocator");
                std::terminate();
            }
        }
    };

    struct allocator_2D_base_legacy{
    protected:
        using T = std::uint32_t;

    public:
        using size_type = T;
        using extent_type = math::vector2<T>;
        using point_type = math::vector2<T>;
        using rect_type = math::rect_ortho<T>;
        using sub_region_type = std::array<rect_type, 3>;

    protected:
        extent_type size{};
        T remainArea{};

        struct node;

        using tree_type = std::map<T, std::multimap<T, node*>>;

        tree_type nodes_XY{};
        tree_type nodes_YX{};

        std::unique_ptr<node> rootNode{std::make_unique<node>(this)};

        std::unordered_map<point_type, node*> allocatedNodes{};

        using ItrOuter = tree_type::iterator;
        using ItrInner = tree_type::mapped_type::iterator;

        struct ItrPair{
            ItrOuter outer{};
            ItrInner inner{};

            [[nodiscard]] auto* value() const{
                return inner->second;
            }

            void erase(tree_type& tree) const{
                auto itr = outer->second.erase(inner);
                if(outer->second.empty()){
                    tree.erase(outer);
                }
            }

            void insert(tree_type& tree, node* node, const T first, const T second){
                outer = tree.try_emplace(first).first;
                inner = outer->second.insert({second, node});
            }

            void locate(tree_type& tree, const T first, const T second){
                outer = tree.lower_bound(first);
                inner = outer->second.lower_bound(second);
            }

            /**
             * @return [rst, possible to find]
             */
            void locateNextInner(const T second){
                inner = outer->second.lower_bound(second);
            }

            bool locateNextOuter(tree_type& tree){
                ++outer;
                if(outer == tree.end()) return false;
                return true;
            }

            [[nodiscard]] bool valid(const tree_type& tree) const noexcept{
                return outer != tree.end() && inner != outer->second.end();
            }
        };

        struct node{
            allocator_2D_base_legacy* packer{};
            node* parent{};

            std::array<std::unique_ptr<node>, 3> subNodes{};

            rect_type capacityRegion{};
            extent_type capturedExtent{};

            ItrPair itrXY{};
            ItrPair itrYX{};

            bool allocated{};
            bool marked{};
            bool childrenCleared{true};

            [[nodiscard]] node() noexcept = default;

            [[nodiscard]] explicit node(allocator_2D_base_legacy* packer) noexcept :
                packer{packer}{}

            [[nodiscard]] node(allocator_2D_base_legacy* packer, node* parent, const rect_type bound) noexcept :
                packer{packer},
                parent{parent},
                capacityRegion{bound}{}

            [[nodiscard]] bool isRoot() const noexcept{
                return parent == nullptr;
            }

            [[nodiscard]] bool isLeaf() const noexcept{
                return subNodes[0] == nullptr && subNodes[1] == nullptr && subNodes[2] == nullptr;
            }

            [[nodiscard]] bool full() const noexcept{
                return capacityRegion.size() == capturedExtent;
            }

            [[nodiscard]] bool empty() const noexcept{
                return capturedExtent.area() == 0;
            }

            [[nodiscard]] bool isAllocated() const noexcept{ return allocated; }

            [[nodiscard]] bool isBranchEmpty() const noexcept{
                if(isAllocated()) return false;

                return std::ranges::all_of(subNodes, [](const std::unique_ptr<node>& node){
                    return node == nullptr || (!node->isAllocated() && node->childrenCleared);
                });
            }

            [[nodiscard]] rect_type allocate(const extent_type size){
                if(size.x > capacityRegion.width() || size.y > capacityRegion.height()){
                    throw std::bad_alloc{};
                }

                allocated = true;
                capturedExtent = size;
                packer->allocatedNodes.insert_or_assign(capacityRegion.get_src(), this);

                if(isLeaf() && !full()){
                    split();
                }

                eraseMark();

                if(!isRoot()){
                    parent->childrenCleared = false;
                }

                return {tags::from_extent, capacityRegion.src, size};
            }

            extent_type deallocate(){
                allocated = false;
                packer->allocatedNodes.erase(capacityRegion.get_src());
                const auto size = capturedExtent;

                //warning: this may have been destructed after this function call
                unsplit(size);

                return size;
            }

            void split(){
                for(const auto& [i, rect] : splitQuad(capacityRegion, capturedExtent) | std::views::enumerate){
                    if(rect.area() == 0) continue;

                    subNodes[i] = std::make_unique<node>(packer, this, rect);
                    subNodes[i]->addMark(rect.size());
                }
            }

            void unsplit(const extent_type size){
                if(isLeaf()){
                    callParentUnsplit();
                } else{
                    if(!tryUnsplit()){
                        //Can only remove self, mark allocatable
                        addMark(size);
                    }
                }
            }

            void callParentUnsplit(){ // NOLINT(*-no-recursion)
                if(!isRoot()){
                    const auto isSelfMerged = parent->tryUnsplit();
                    if(!isSelfMerged){
                        //maximum merge at self, mark allocatable
                        resetMark(capacityRegion.size());
                    }
                } else{
                    resetMark(capacityRegion.size());
                }
            }

            /**
             * @brief
             * @return true if merge is successful
             */
            bool tryUnsplit(){ // NOLINT(*-no-recursion)
                if(isBranchEmpty()){
                    childrenCleared = true;
                    for(auto&& subNode : subNodes){
                        if(subNode == nullptr) continue;
                        subNode->eraseMark();
                        subNode.reset();
                    }

                    callParentUnsplit();

                    return true;
                }

                return false;
            }

            void eraseMark(){
                if(!marked) return;
                itrXY.erase(packer->nodes_XY);
                itrYX.erase(packer->nodes_YX);

                marked = false;
            }

            void addMark(const extent_type size){
                itrXY.insert(packer->nodes_XY, this, size.x, size.y);
                itrYX.insert(packer->nodes_YX, this, size.y, size.x);
                marked = true;
            }

            void resetMark(const extent_type size){
                if(!marked) return;
                itrXY.erase(packer->nodes_XY);
                itrYX.erase(packer->nodes_YX);

                itrXY.insert(packer->nodes_XY, this, size.x, size.y);
                itrYX.insert(packer->nodes_YX, this, size.y, size.x);
            }
        };

        friend node;

        /**
         * @code
         *    array[0]  |  array[2]
         * -------------|------------
         *      Box     |  array[1]
         * @endcode
         */
        [[nodiscard]] static constexpr sub_region_type splitQuad(const rect_type bound, const extent_type box) noexcept{
            const auto [rx, ry] = bound.size() - box;

            return sub_region_type{
                rect_type{tags::from_extent, bound.vert_00().add(0, box.y), box.x, ry},
                rect_type{tags::from_extent, bound.vert_00().add(box.x, 0), rx, box.y},
                rect_type{tags::from_extent, bound.vert_00() + box, rx, ry},
            };
        }

    public:
        [[nodiscard]] allocator_2D_base_legacy() = default;

        [[nodiscard]] explicit allocator_2D_base_legacy(const extent_type size) : size{size}, remainArea{size.area()} {
            rootNode->capacityRegion.set_size(size);
            rootNode->addMark(size);
        }
    };


    export class allocator_2D_legacy : public allocator_2D_base_legacy{
    public:
        using allocator_2D_base_legacy::allocator_2D_base_legacy;

        //TODO double key -- with allocation region depth
        [[nodiscard]] std::optional<rect_type> allocate(const extent_type size) {
            auto* node = getValidNode(size);

            if(!node)return std::nullopt;

            remainArea -= size.area();

            return node->allocate(size);
        }

        void deallocate(const point_type src) noexcept {
            if(const auto itr = allocatedNodes.find(src); itr != allocatedNodes.end()) {
                remainArea += itr->second->deallocate().area();
            }
        }

        [[nodiscard]] extent_type extent() const noexcept{ return size; }

        [[nodiscard]] T get_valid_area() const noexcept{ return remainArea; }

        allocator_2D_legacy(const allocator_2D_legacy& other) = delete;

        allocator_2D_legacy(allocator_2D_legacy&& other) noexcept
            : allocator_2D_base_legacy{std::move(other)}{
            changePackerToThis();
        }

        allocator_2D_legacy& operator=(const allocator_2D_legacy& other) = delete;

        allocator_2D_legacy& operator=(allocator_2D_legacy&& other) noexcept{
            if(this == &other) return *this;
            allocator_2D_base_legacy::operator =(std::move(other));
            changePackerToThis();
            return *this;
        }

    private:
        node* getValidNode(const extent_type size){
            assert(size.area() > 0);

            if(remainArea < size.area()) {return nullptr;}

            ItrPair itrPairXY{nodes_XY.lower_bound(size.x)};
            ItrPair itrPairYX{nodes_YX.lower_bound(size.y)};

            node* node{};

            bool possibleX{itrPairXY.outer != nodes_XY.end()};
            bool possibleY{itrPairYX.outer != nodes_YX.end()};

            while(true)
            {
                if(!possibleX && !possibleY)break;

                if(possibleX){
                    itrPairXY.locateNextInner(size.y);
                    if (itrPairXY.valid(nodes_XY)){
                        node = itrPairXY.value();
                        break;
                    }else{
                        possibleX = itrPairXY.locateNextOuter(nodes_XY);
                    }
                }

                if(possibleY){
                    itrPairYX.locateNextInner(size.x);
                    if (itrPairYX.valid(nodes_YX)){
                        node = itrPairYX.value();
                        break;
                    }else{
                        possibleY = itrPairYX.locateNextOuter(nodes_YX);
                    }
                }
            }

            return node;
        }

        template <std::regular_invocable<allocator_2D_legacy&, node&> Fn>
        void dfsEach_impl(Fn fn, node* current){
            std::invoke(fn, *this, *current);

            for (auto && subNode : current->subNodes){
                if(subNode){
                    this->dfsEach_impl<Fn>(fn, subNode.get());
                }
            }
        }

        template <std::regular_invocable<allocator_2D_legacy&, node&> Fn>
        void dfsEach(Fn fn){
            this->dfsEach_impl<Fn>(std::move(fn), rootNode.get());
        }

        void changePackerToThis(){
            dfsEach([](allocator_2D_legacy& packer, node& node){
                node.packer = &packer;
            });
        }
    };

}