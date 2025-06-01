add_moduledirs("modules")

add_defines("WIN32_LEAN_AND_MEAN")

add_shflags("/NOIMPLIB", {tools = "link"})
if is_mode("release") then
    -- remove debug dir
    add_shflags("/EMITPOGOPHASEINFO", {force = true, tools = "link"})
end

target("ark-status-point")
    set_kind("shared")
    add_files("*.cpp")
    add_packages("microsoft-detours", "fmt")
    add_syslinks("user32")

    add_deps("dwmapi", {inherit = false})

    on_run(function (target)
        import("core.project.depend")

        local ark_dir = get_config("ark_dir")
        assert(ark_dir or ark_dir == "", "Please specify the ShooterGame.exe directory using --ark_dir")

        depend.on_changed(function()
            os.vcp(target:targetfile(), ark_dir)
        end, {
            files = target:targetfile()
        })

        local dwmapi = target:dep("dwmapi"):targetfile()
        depend.on_changed(function()
            os.vcp(dwmapi, ark_dir)
        end, {
            files = dwmapi
        })
        os.exec(path.join(ark_dir, "ShooterGame.exe"))
    end)

target("dwmapi")
    add_rules("c")
    add_rules("dll_hijacking", {import = "ark-status-point.dll"})
    add_files("C:/Windows/System32/dwmapi.dll")
