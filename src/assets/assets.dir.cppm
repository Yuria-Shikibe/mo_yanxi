export module mo_yanxi.assets.directories;

import std;
export import mo_yanxi.io.file;
// export import Core.FilePlain;
// export import Core.FilePlain;


namespace mo_yanxi::assets{
	namespace dir{
	    export {
	    	inline io::file prop_root;

	        inline io::file assets;

	        inline io::file shader;
	        inline io::file shader_spv;
	        inline io::file shader_src;

	        inline io::file texture;
	        inline io::file font;
	        inline io::file svg;
	        inline io::file bundle;
	        inline io::file sound;


	        inline io::file cache;

	        // inline io::file data;
	        // inline io::file settings;
	        //
	        // inline io::file game;
	        //
	        // inline io::file texCache;
	    }

		void patch(const io::file& dir){
			if(!dir.exist()){
				dir.create_dir_quiet();
			}
		}
	}

	export void load_dir(const std::filesystem::path& root);
}
