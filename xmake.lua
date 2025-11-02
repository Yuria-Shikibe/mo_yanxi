add_rules("mode.debug", "mode.release")
set_arch("x64")
set_encodings("utf-8")
set_project("mo_yanxi")


set_toolchains("msvc")

if is_mode("debug") then
    set_runtimes("MDd")
else
    set_runtimes("MD")
end

add_requires("glfw")
add_requires("spirv-cross")
if is_mode("debug") then
    add_requireconfs("spirv-cross", {configs = {runtimes = "MDd", debug = true}})
else
    add_requireconfs("spirv-cross", {configs = {runtimes = "MD", debug = false}})
end

--[[ , {configs = {
                             toolchains = "clang-cl"
                         }}) ]]

add_requires("msdfgen", {
    configs = {
        openmp = true,
        extensions = true ,
--         toolchains = "clang-cl"
    }
 })
add_requires("freetype")
add_requires("nanosvg")
add_requires("protobuf-cpp", {
    configs = {
--         toolchains = "clang-cl",
        zlib = true,
    }
})

target("protobuf_gen")
    set_kind("static")
    set_languages("cxx17")
    add_packages("protobuf-cpp", {public = true})
    add_rules("protobuf.cpp")
    add_files("generate/srl/*.cc")
target_end()

target("mo_yanxi")
    set_kind("binary")
    set_extension(".exe")
    set_languages("c++latest")
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
    --add_deps("magic_enum")

    add_defines("_MSVC_STL_HARDENING=1")
    add_defines("_MSVC_STL_DESTRUCTOR_TOMBSTONES=1")
    add_defines("MAGIC_ENUM_USE_STD_MODULE")


    set_warnings("all")
    set_warnings("pedantic")
--     add_vectorexts("avx", "avx2", "avx512")
    add_vectorexts("avx", "avx2"--[[ , "avx512" ]])
--     add_vectorexts("sse", "sse2", "sse3", "ssse3", "sse4.2")

    add_cxflags("/diagnostics:column")
    add_cxflags("/FC")
    add_cxxflags("/Zc:__cplusplus")
    add_cxxflags("/permissive-")

    add_files("src/**.cppm")
    add_files("src/**.ixx")
    add_files("src/**.cpp")

    add_packages("glfw")
    add_packages("spirv-cross")
    add_packages("msdfgen")
    add_packages("freetype")

    add_packages("nanosvg")

    add_includedirs("generate")
    add_includedirs("src")

    add_includedirs("submodules/magic_enum/include")
    add_files("submodules/magic_enum/module/**.cppm")

    add_includedirs("submodules/VulkanMemoryAllocator/include")
    add_includedirs("submodules/plf_hive")
    add_includedirs("submodules/small_vector/source/include")
    add_includedirs("submodules/tim_sort/include")
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

        -- 移除不需要的命令
        local cleaned = content:gsub("add_custom_command%(%s-.-%)%s*\n", "")
                               :gsub("add_custom_command%(%s-.-%b())%s*\n", "")
                               :gsub("set_source_files_properties%(%s-.-%)%s*\n", "")
                               :gsub("set_source_files_properties%(%s-.-%b())%s*\n", "")
                                :gsub("target_compile_features%(%s-.-cxx_std_26%s-%)%s*\n", "")
--                                 :gsub("target_compile_options%(%s-.-%)%s*\n", "")

        ---- 在 project() 后插入 C++ 标准设置
        local found_project = false
        local new_content = cleaned:gsub("(project%s*%b())", function(capture)
            found_project = true
            return capture .. "\nset(CMAKE_CXX_STANDARD 23)"
        end)

        if not found_project then
            -- 如果找不到 project()，则在文件开头插入
            new_content = "set(CMAKE_CXX_STANDARD 23)\n" .. cleaned
        end

        io.writefile(cmake_file, new_content)
        print("Removed custom commands and set C++ standard to 23 in CMakeLists.txt")
    end)

    set_menu{usage = "create cmakelists"}
task_end()


task("gen_proto_buf")
    on_run(function ()
        os.exec("py ./build_scripts/proto_builder.py -i ./generate/srl/pb.schema -o ./generate/srl")
    end)

    set_menu{usage = "create protobuf cpp codes"}
task_end()

task("gen_slang")
    on_run(function ()
        os.exec("py ./build_scripts/slang_builder.py ./prop/util/bin/slangc.exe ./prop/assets/shader/spv ./prop/assets/shader/config.json")
    end)

    set_menu{usage = "compile slang to spirv"}
task_end()
