set_project("ark-status-point")

set_version("0.0.1")

set_xmakever("2.9.9")

set_allowedplats("windows")
set_allowedarchs("x64")
set_allowedmodes("debug", "release")

includes("src", "xmake")

-- fixed config
set_languages("c++20")
add_rules("mode.debug", "mode.release")

if is_mode("debug") then
    add_defines("PROJECT_DEBUG")
elseif is_mode("release") then
    set_optimize("smallest")
end

if is_plat("windows") then
    add_defines("UNICODE", "_UNICODE")
end

set_encodings("utf-8")

-- dynamic config
if has_config("dev") then
    set_policy("compatibility.version", "3.0")
    set_policy("build.ccache", true)

    add_rules("plugin.compile_commands.autoupdate", {lsp = "clangd", outputdir = "build"})

    set_warnings("all")

    if is_plat("windows") then
        set_runtimes("MD")
        add_cxflags("/permissive-", {tools = "cl"})
    end
end
