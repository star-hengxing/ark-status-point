import("core.base.option")
import("core.base.global")

local options = {
    {nil, "ci", "k", nil, "Is ci evnvironment?"},
}

function _get_current_commit_hash()
    return os.iorunv("git rev-parse --short HEAD"):trim()
end

function _get_current_tag()
    return os.iorunv("git describe --tags --abbrev=0"):trim()
end

function main(...)
    local opt = option.parse({...}, options, "xmake l xmake/release.lua --ci") or {}

    print("current tag: ", tag)
    print("current commit: ", current_commit)

    os.vexecv(os.programfile(), {"config", "--clean", "--yes", "--dev=n", "--policies=build.optimization.lto:y"})
    os.vexecv(os.programfile(), {"pack"})

    if not opt.ci then
        return
    end

    local envs = {}
    if global.get("proxy") then
        envs.HTTPS_PROXY = global.get("proxy")
    end

    local binary = "build/xpack/release/ark-status-point.7z"
    os.vexecv("gh", {"release", "upload", tag, binary}, {envs = envs})
end
