module;

module mo_yanxi.graphic.bitmap;

import mo_yanxi.io.image;
import mo_yanxi.io.file;

mo_yanxi::graphic::bitmap::bitmap(std::string_view path){
	int width, height, bpp;
	const auto ptr = io::image::load_png(path, width, height, bpp, channels);

	create(width, height);
	std::memcpy(data(), ptr.get(), width * height * bpp);
}

void mo_yanxi::graphic::bitmap::write(const std::string_view path, bool autoCreateFile) const{
	if(area() == 0) return;
	//TODO...

	io::file file{path};
	if(!file.exist()){
		bool result = false;

		if(autoCreateFile) result = file.create_file();

		if(!file.exist() || !result) throw std::runtime_error{"Inexist File!"};
	}

	//OPTM ...
	// ReSharper disable once CppTooWideScopeInitStatement
	std::string ext = file.extension();

	if(path.ends_with(".bmp")){
		io::image::write_bmp(path, width(), height(), channels, reinterpret_cast<const unsigned char*>(data()));
	}

	io::image::write_png(path, width(), height(), channels, reinterpret_cast<const unsigned char*>(data()));
}
