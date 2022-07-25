project('vwa-code', 'cpp')

glfw_dep = dependency('glfw3')
glad_dep = subproject('glad').get_variable('glad_dep')

executable('vwa-code', 'source/main.cpp',
    dependencies: [glfw_dep, glad_dep])
