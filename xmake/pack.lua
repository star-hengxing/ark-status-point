includes("@builtin/xpack")

-- build/xpack/release/ark-status-point.7z
xpack("release")
    set_formats("zip")
    set_extension(".7z")
    set_basename("ark-status-point")

    add_targets("ark-status-point")

    set_prefixdir("ark-status-point")

    set_bindir("")
