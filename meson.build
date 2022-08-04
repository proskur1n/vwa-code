project('vwa-code', 'cpp', default_options: ['cpp_std=c++17', 'build.cpp_std=c++17'])

glfw_dep = dependency('glfw3')
glad_dep = subproject('glad').get_variable('glad_dep')
glm_dep = dependency('glm')
imgui_dep = subproject('imgui').get_variable('imgui_dep')

executable('vwa-code', 'source/obj_parser/parser.cpp', 'source/main.cpp',
    cpp_args: [
        '-DGLM_FORCE_XYZW_ONLY',
        '-DGLM_FORCE_CTOR_INIT'
    ],
    include_directories: 'source',
    dependencies: [glfw_dep, glad_dep, glm_dep, imgui_dep]
)
