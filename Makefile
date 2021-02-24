CC := $(CC) $(CFLAGS)
CXX := $(CXX) $(CXXFLAGS)

CFILES = glad/glad.c one-stl/one-stl.c
CXXFILES = imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp\
		   imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp\
		   imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/ImGuizmo.cpp\
		   program.cpp mesh.cpp main.cpp

OBJS = $(CFILES:%=inter/%.o) $(CXXFILES:%=inter/%.o)

vwa-code: $(OBJS)
	$(CXX) $(OBJS) -lglfw -o $@

clean:
	rm -rf inter vwa-code imgui.ini

inter/%.cpp.o: %.cpp *.h
	@mkdir -p inter
	$(CXX) -c $< -o $@ -Ideps -std=c++11 -Wall -Wpedantic

inter/glad/glad.c.o: deps/glad/glad.c deps/glad/*.h
	@mkdir -p inter/glad
	$(CC) -c $< -o $@

inter/one-stl/one-stl.c.o: deps/one-stl/one-stl.c deps/one-stl/*.h
	@mkdir -p inter/one-stl
	$(CC) -c $< -o $@

inter/imgui/%.o: deps/imgui/% deps/imgui/*.h
	@mkdir -p inter/imgui
	$(CXX) -c $< -o $@ -DIMGUI_IMPL_OPENGL_LOADER_GLAD -Ideps
