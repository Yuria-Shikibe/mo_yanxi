module;

#include <cassert>

export module mo_yanxi.font.manager;

export import mo_yanxi.font;

import mo_yanxi.graphic.image_region;
import mo_yanxi.graphic.image_atlas;

import mo_yanxi.math.vector2;

import std;
import mo_yanxi.heterogeneous;

export namespace mo_yanxi::font{

	struct glyph : graphic::cached_image_region{
		glyph_metrics metrics{};
		glyph_ptr meta{};

		[[nodiscard]] constexpr glyph() = default;

		[[nodiscard]] constexpr glyph(const glyph_ptr& meta, graphic::allocated_image_region& region)
			: cached_image_region{region}, metrics{meta->metrics}, meta{meta}{}

		[[nodiscard]] constexpr explicit(false) glyph(const glyph_ptr& meta)
			: cached_image_region{nullptr}, metrics{meta->metrics}, meta{meta}{}

	};

	struct font_manager{
	private:
		graphic::image_page* fontPage{};
		string_hash_map<font_face> fontFaces{};

	public:
		[[nodiscard]] font_manager() = default;

		[[nodiscard]] explicit font_manager(graphic::image_atlas& atlas, std::string_view fontPageName = "font") :
			fontPage(&atlas.create_image_page(fontPageName))
		{}

		[[nodiscard]] graphic::image_page& page() const noexcept{
			assert(fontPage != nullptr);
			return *fontPage;
		}

		[[nodiscard]] glyph get_glyph_exact(font_face& ff, const glyph_identity key){
			auto name = ff.format(key.code, key.size);
			const auto ptr  = ff.obtain(key.code, key.size);
			if(ptr->bitmap.area() && !is_space(key.code)){
				const auto aloc = page().register_named_region(std::move(name), ptr->bitmap);
				return glyph{ptr, aloc.first};
			}

			return glyph{ptr};
		}

		[[nodiscard]] glyph get_glyph(const std::string_view fontKeyName, const char_code code, const unsigned size){
			if(const auto face = find_face(fontKeyName)){
				return get_glyph_exact(*face, glyph_identity{code, {static_cast<glyph_size_type::value_type>(get_snapped_size(size)), 0}});
			}

			return {};
		}

		[[nodiscard]] glyph get_glyph_exact(const std::string_view fontKeyName, const glyph_identity key){
			if(const auto face = find_face(fontKeyName)){
				return get_glyph_exact(*face, key);
			}

			return {};
		}

		[[nodiscard]] glyph get_glyph(font_face& ff, const char_code code, const unsigned size){
			return get_glyph_exact(ff, glyph_identity{code, {static_cast<glyph_size_type::value_type>(get_snapped_size(size)), 0}});
		}

		font_face& register_face(std::string_view keyName, std::string_view fontName){
			return fontFaces.insert_or_assign(keyName, font_face{fontName}).first->second;
		}

		[[nodiscard]] font_face* find_face(std::string_view keyName) noexcept{
			return fontFaces.try_find(keyName);
		}

		[[nodiscard]] const font_face* find_face(std::string_view keyName) const noexcept{
			return fontFaces.try_find(keyName);
		}

	};
}
