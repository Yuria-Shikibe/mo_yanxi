add_rules("mode.debug", "mode.release")

set_arch("x64")

set_policy("build.c++.modules", true)
set_policy("build.ccache", true)

if is_mode("debug") then
    set_runtimes("MDd")
    add_defines("DEBUG_CHECK=1")
else
    set_runtimes("MD")
    add_defines("DEBUG_CHECK=0")
end

set_warnings("all")
set_encodings("utf-8")

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

if is_mode("release") then
    set_symbols("debug")
    set_optimize("fastest")
end

set_toolchains("msvc")
add_linkdirs("C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64")


target("mo_yanxi")
    set_kind("binary")
    set_extension(".exe")
    set_languages("c++23")


    if is_plat("windows") then
        add_links("kernel32", "user32")  -- Windows API库
        add_syslinks("advapi32", "shell32")
    end

    if is_mode("debug") then
        add_defines("_DEBUG")
        add_cxflags("/RTCsu", { force = true })
    end

    add_cxflags("/FC", { force = true })
    add_cxflags("/diagnostics:column")
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

    add_includedirs("submodules/VulkanMemoryAllocator/include")
    add_includedirs("submodules/plf_hive")
    add_includedirs("submodules/small_vector/source/include")
    add_includedirs("includes")


    -- Windows 平台特定配置
    if is_plat("windows") then
        -- 获取 Vulkan SDK 环境变量
        local vulkan_sdk = os.getenv("VULKAN_SDK")

        if not vulkan_sdk then
            raise("Vulkan SDK not found!")
        end

        -- 添加包含目录
        add_includedirs(path.join(vulkan_sdk, "Include"))

        -- 根据架构选择库目录
        if is_arch("x64") then
            add_linkdirs(path.join(vulkan_sdk, "Lib"))
        else
            add_linkdirs(path.join(vulkan_sdk, "Lib32"))
        end

        -- 链接 Vulkan 主库
        add_links("vulkan-1")

        -- 如果需要 shaderc 等额外库，可以添加：
        add_links("shaderc_shared")

        add_defines("VK_USE_64_BIT_PTR_DEFINES=1")
    end
target_end()

task("gen_cmake")
    on_run(function ()
        os.exec("xmake project -k cmake")

        local cmake_file = path.join(os.projectdir(), "CMakeLists.txt")

        local content = io.readfile(cmake_file) or ""
        local cleaned = content:gsub("add_custom_command%(%s-.-%)%s*\n", "")
                               :gsub("add_custom_command%(%s-.-%b())%s*\n", "")
                               :gsub("set_source_files_properties%(%s-.-%)%s*\n", "")
                               :gsub("set_source_files_properties%(%s-.-%b())%s*\n", "")

        io.writefile(cmake_file, cleaned)

        print("Removed all add_custom_command from CMakeLists.txt")
    end)

    set_menu{usage = "create cmakelists", description = "", options = {}}
task_end()

-- rule("cmake")


--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

