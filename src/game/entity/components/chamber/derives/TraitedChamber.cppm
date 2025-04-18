/*
//
// Created by Matrix on 2024/11/10.
//

export module Game.Chamber.TraitedChamber;

export import Game.Chamber;
export import Game.Content.Trait;
export import Game.Content.EntityTrait;
import std;

namespace Game::Chamber{
	export
	template <typename Impl, typename BuildTy = Building<void>>
	struct ChamberTrait_Interface{
		using EntityType = typename BuildTy::EntityType;

		template <typename DataType>
		struct BuildType_Base : public BuildTy{
			DataType data{};
			Game::Traits::TraitReference<Impl> trait{};

			[[nodiscard]] explicit BuildType_Base(const Traits::TraitReference<Impl>& trait)
				: trait(trait){
			}
		};

		/** @brief Type To Export#1#
		using BuildType = void;


		auto create() const {
			return ext::referenced_ptr<typename Impl::BuildType>{std::in_place, static_cast<const Impl&>(*this)};
		}
	};

	export
	template <typename T>
		requires requires{
			typename T::BuildType;
			typename T::DataType;
			typename T::EntityType;
		}
	struct ChamberTrait{
		using BuildType = typename T::BuildType;
		using DataType = typename T::DataType;
		using EntityType = typename T::EntityType;

		[[nodiscard]] static ext::referenced_ptr<BuildType> create(const T& trait) noexcept{
			return trait.create();
		}
	};

}
*/
