handle_glsl_files = function(opt, opath, ipaths)
    local shader_extensions = { "vert", "frag", "comp", "geom" }

    for _, ext in ipairs(shader_extensions) do
        local glslc_command = "\"libs/glslangValidator.exe\" -V %{file.relpath} -o %{file.relpath}.spv"

        filter("files:**." .. ext)
            buildmessage("Compiling GLSL: '%{file.name}'")
            buildcommands(glslc_command)
            buildoutputs("%{file.relpath}.spv")
        filter("*")
    end
end
