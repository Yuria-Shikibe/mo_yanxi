module;

#include <cassert>

export module mo_yanxi.game.meta.content;

export import mo_yanxi.game.meta.content_ref;
import mo_yanxi.flat_seq_map;
import mo_yanxi.heterogeneous;
import mo_yanxi.heterogeneous.open_addr_hash;
import std;

namespace mo_yanxi::game::meta{
	export struct content_add_error : std::runtime_error{
		using std::runtime_error::runtime_error;
	};

	struct content_registry_base{
	private:
		bool frozen{};
		// content_tag tag_;
		std::type_index dst_type_index;


	protected:
		[[nodiscard]] explicit content_registry_base(const std::type_index& dst_type_index) :
			dst_type_index(dst_type_index){
		}


	public:
		content_registry_base(const content_registry_base& other) = delete;
		content_registry_base(content_registry_base&& other) noexcept = delete;
		content_registry_base& operator=(const content_registry_base& other) = delete;
		content_registry_base& operator=(content_registry_base&& other) noexcept = delete;

		virtual ~content_registry_base() = default;

		virtual content_ref<void> find(std::string_view name) = 0;

		[[nodiscard]] std::type_index content_type_index() const noexcept{
			return dst_type_index;
		}

		[[nodiscard]] bool is_frozen() const{
			return frozen;
		}

		void set_frozen(bool frozen) noexcept{
			this->frozen = frozen;
		}
	};

	export
	template <typename T>
	struct content_registry : content_registry_base{

		using content_type = T;
		using cont_type = flat_seq_map<std::string, content_type, mo_yanxi::transparent::string_comparator_of<std::less>>;

	private:
		cont_type contents{};

	public:
		[[nodiscard]] explicit content_registry()
			: content_registry_base(typeid(T)){
		}

		content_type& add(std::string_view name, content_type&& content){
			if(is_frozen()){
				throw content_add_error{"add content after frozen may cause dangling"};
			}

			auto [itr, suc] = contents.insert_unique(name, std::move(content));
			if(!suc){
				throw content_add_error{std::format("duplicated content name: {}", name)};
			}
			return itr->value;
		}

		content_ref<void> find(std::string_view name) override{
			if(auto rst = find_raw(name)){
				return {name, rst};
			}else{
				return {};
			}
		}

		content_type* find_raw(std::string_view name){
			if(!is_frozen()){
				throw content_add_error{"use content before frozen may cause dangling"};
			}

			return contents.find_unique(name);
		}

	};

	export
	struct content_manager{
	private:
		using map = type_fixed_hash_map<std::unique_ptr<content_registry_base>>;
		map contents{};

	public:

		void freeze() const noexcept{
			for (const auto& ty : contents | std::views::values){
				ty->set_frozen(true);
			}
		}

		template <typename ContentTy>
		std::remove_cvref_t<ContentTy>& add(std::string_view name, ContentTy&& content){
			if(auto it = contents.find(typeid(ContentTy)); it != contents.end()){
				return static_cast<content_registry<std::remove_cvref_t<ContentTy>>>(it->second.get())->add(name, std::forward<ContentTy>(content));
			}

			auto& chunk = register_type<ContentTy>();
			return chunk.add(name, std::forward<ContentTy>(content));
		}

		template <typename T>
		content_registry<T>& register_type(){
			auto [itr, suc] = contents.try_emplace(typeid(T), std::make_unique<content_registry<T>>());
			if(!suc){

				throw content_add_error{std::format("Duplicated content on type", typeid(T).name())};
			}else{
				return itr->second;
			}
		}

		template <typename T>
		T* find(std::string_view name) noexcept{
			if(auto itr = contents.find(typeid(T)); itr != contents.end()){
				auto* rg = itr->second.get();
				return static_cast<content_registry<std::remove_cvref_t<T>>>(rg).find_raw(name);
			}

			return nullptr;
		}
	};
}