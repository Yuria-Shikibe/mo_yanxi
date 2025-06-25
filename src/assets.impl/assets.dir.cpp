module mo_yanxi.assets.directories;


void mo_yanxi::assets::load_dir(const std::filesystem::path& root){
    using namespace dir;

    prop_root = root;
    patch(prop_root);

    dir::assets = prop_root.sub_file("assets");
    patch(dir::assets);

    shader = dir::assets.sub_file("shader");
    patch(shader);

    shader_spv = shader.sub_file("spv");
    patch(shader_spv);

    shader_src = shader.sub_file("src");
    patch(shader_src);

    texture = dir::assets.sub_file("texture");
    patch(texture);

    font = dir::assets.sub_file("fonts");
    patch(font);

    sound = dir::assets.sub_file("sounds");
    patch(sound);

    svg = dir::assets.sub_file("svg");
    patch(svg);

    bundle = dir::assets.sub_file("bundles");
    patch(bundle);


    cache = prop_root.sub_file("cache");
    patch(cache);

    // texCache = cache.subFile("tex");
    // patch(texCache);
    //
    // game = dir::assets.subFile("game");
    // patch(game);
    //
    // data = files.findDir("resource").subFile("data");
    // patch(data);
    //
    // settings = data.subFile("settings");
    // patch(settings);
    //
    // screenshot = mainTree.find("screenshots"); //TODO move this to other places, it doesn't belong to assets!
    // patch(texCache);
}
