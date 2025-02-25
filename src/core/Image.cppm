module;

// ReSharper disable once CppUnusedIncludeDirective
// #include "stbi_io.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <native/stbi/stbi_image_write.h>
#include <native/stbi/stbi_image.h>

export module mo_yanxi.io.image;

import std;

namespace mo_yanxi::io::image{
	export struct stbi_deleter{
		void operator ()(stbi_uc* ptr) const noexcept{
			STBI_FREE(ptr);
		}
	};

	export int write_png(char const* filename, const int x, const int y, const int comp, const void* data,
	                     const int stride_bytes){
		return stbi_write_png(filename, x, y, comp, data, stride_bytes);
	}

	void setFlipVertically_load(const bool flag){
		stbi_set_flip_vertically_on_load(flag);
	}

	void setFlipVertically_write(const bool flag){
		stbi_flip_vertically_on_write(flag);
	}

	//byte per pixel
	//RGBA ~ 4 ~ 32 [0 ~ 255]
	//RGB ~ 3
	export
	[[nodiscard]] std::unique_ptr<stbi_uc, stbi_deleter> load_png(const std::string_view path, int& width, int& height,
	                                                              int& bpp, const int requiredBpp = 4){
		int w, h, b;

		const auto data = stbi_load(path.data(), &w, &h, &b, requiredBpp);
		width = w;
		height = h;
		bpp = requiredBpp;
		return std::unique_ptr<stbi_uc, stbi_deleter>{data};
	}

	std::string_view get_exception(){
		return stbi_failure_reason();
	}

	export int write_png(const std::string_view path, const unsigned int width, const unsigned int height,
	                     const unsigned int bpp, const unsigned char* data, const int stride = 0){
		return stbi_write_png(path.data(), static_cast<int>(width), static_cast<int>(height), static_cast<int>(bpp),
		                      data, stride);
	}

	export int write_bmp(const std::string_view path, const unsigned int width, const unsigned int height,
	                     const unsigned int bpp, const unsigned char* data){
		return stbi_write_bmp(path.data(), static_cast<int>(width), static_cast<int>(height), static_cast<int>(bpp),
		                      data);
	}
}
