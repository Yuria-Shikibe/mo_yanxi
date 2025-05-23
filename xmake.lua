add_rules("mode.debug", "mode.release")
set_arch("x64")
set_encodings("utf-8")

set_policy("build.ccache", true)

set_toolchains("msvc")
add_cxflags("/diagnostics:column")

if is_mode("debug") then
    set_runtimes("MDd")
else
    set_runtimes("MD")
end


set_warnings("all")

add_requires("glfw")
add_requires("msdfgen", {
    configs = {
        openmp = true,
        extensions = true ,
        cxxflags = "/DMSDFGEN_USE_CPP11"
    }
 })
add_requires("freetype")
add_requires("nanosvg")
add_requires("protobuf-cpp", {
    configs = {
        zlib = true,
    }
})


target("protobuf_gen")
    set_kind("static")
    set_languages("c++17")
    add_packages("protobuf-cpp", {public = true})
    add_rules("protobuf.cpp")
    add_files("generate/srl/*.cc")
target_end()

target("mo_yanxi")
    set_kind("binary")
    set_extension(".exe")
    set_languages("c++23")
    set_policy("build.c++.modules", true)

    if is_mode("debug") then
        add_cxflags("/RTCsu")
        add_defines("DEBUG_CHECK=1")
    else
        set_symbols("debug")
        add_cxflags("/GL")
        add_ldflags("/LTCG")

        add_defines("DEBUG_CHECK=0")
    end

    add_deps("protobuf_gen")

    add_defines("_MSVC_STL_HARDENING=1")
    add_defines("_MSVC_STL_DESTRUCTOR_TOMBSTONES=1")

    add_cxflags("/Zc:__cplusplus")
    add_cxflags("/FC")
    add_cxflags("/arch:AVX")
    add_cxflags("/arch:AVX2")

    add_cxflags("/wd4244")
    add_cxflags("/wd4100")
    add_cxflags("/wd4458")
    add_cxflags("/wd4267")
    add_cxflags("/wd4189")
    add_cxflags("/wd5030")
    add_cxflags("/bigobj")

    add_files("src/**.cppm")
    add_files("src/**.ixx")
    add_files("src/**.cpp")

    add_packages("glfw")
    add_packages("msdfgen")
    add_packages("freetype")

    add_packages("nanosvg")
    add_packages("protobuf-cpp")

    add_includedirs("generate")
    add_includedirs("src")
    add_includedirs("submodules/VulkanMemoryAllocator/include")
    add_includedirs("submodules/plf_hive")
    add_includedirs("submodules/small_vector/source/include")
    add_includedirs("submodules/stb")

    local vulkan_sdk = os.getenv("VULKAN_SDK")

    if not vulkan_sdk then
        raise("Vulkan SDK not found!")
    end

    add_includedirs(path.join(vulkan_sdk, "Include"))
    add_linkdirs(path.join(vulkan_sdk, "Lib"))

    add_links("vulkan-1")
    add_links("shaderc_shared")

    add_defines("VK_USE_64_BIT_PTR_DEFINES=1")
target_end()

task("gen_ide_hintonly_cmake")
    on_run(function ()
        os.exec("xmake project -k cmakelists")

        local cmake_file = path.join(os.projectdir(), "CMakeLists.txt")

        local content = io.readfile(cmake_file) or ""
        local cleaned = content:gsub("add_custom_command%(%s-.-%)%s*\n", "")
                               :gsub("add_custom_command%(%s-.-%b())%s*\n", "")
                               :gsub("set_source_files_properties%(%s-.-%)%s*\n", "")
                               :gsub("set_source_files_properties%(%s-.-%b())%s*\n", "")

        io.writefile(cmake_file, cleaned)

        print("Removed all add_custom_command from CMakeLists.txt")
    end)

    set_menu{usage = "create cmakelists"}
task_end()


task("gen_proto_buf")
    on_run(function ()
        os.exec("py ./build_scripts/proto_builder.py -i ./generate/srl/pb.schema -o ./generate/srl")
    end)

    set_menu{usage = "create protobuf cpp codes"}
task_end()
