#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/ImGuizmo.h>
#include "program.h"
#include "framebuffer.h"
#include "util.h"

const auto MOUSE_ACCEL = 0.01f;
const auto MOVEMENT_SPEED = 0.04f;

struct {
	GLFWwindow *window;

	std::vector<mesh> objects;
	mesh *selected_object {nullptr};

	framebuffer default_fbo;
	camera primary_camera;
	camera shadow_camera;
	camera *active_camera = &primary_camera;

	shadow_renderer shadow_renderer;
	color_renderer color_renderer;

	bool show_gui {true};
	ImGuizmo::OPERATION guizmo_op {ImGuizmo::TRANSLATE};
	// TODO onject was selected / draw guizmo
} G;

void on_key_input(GLFWwindow *win, int key, int scancode, int action, int mods) {
	if (ImGui::GetIO().WantCaptureKeyboard) {
		return;
	}

	if (key == GLFW_KEY_G && action == GLFW_RELEASE) {
		G.show_gui = !G.show_gui;
	}

	struct {
		int key;
		ImGuizmo::OPERATION op;
	} guizmo_keys[] = {
		{ GLFW_KEY_1, ImGuizmo::TRANSLATE },
		{ GLFW_KEY_2, ImGuizmo::ROTATE },
		{ GLFW_KEY_3, ImGuizmo::SCALE },
	};
	for (auto k : guizmo_keys) {
		if (key == k.key && action == GLFW_RELEASE) {
			G.guizmo_op = k.op;
			break;
		}
	}
}

void on_mouse_button(GLFWwindow *window, int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		glfwSetInputMode(window, GLFW_CURSOR,
			action == GLFW_PRESS ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		return;
	}
	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		double xpos;
		double ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		/* OpenGL coordinates start in the lower left corner */
		ypos = G.default_fbo.height - ypos;

		G.default_fbo.bind_for_rendering();
		GLint index;
		glReadPixels(xpos, ypos, 1, 1, GL_STENCIL_INDEX, GL_INT, &index);

		G.selected_object = nullptr;
		for (auto &obj : G.objects) {
			if (obj.get_index() == index) {
				G.selected_object = &obj;
				break;
			}
		}
	}
}

void on_window_resize(GLFWwindow *window, int width, int height) {
	// TODO
	// set width and height of default framebuffer
	// call calc_projection() on active camera
}

GLFWwindow *initialize(int width, int height, char const *title) {
	if (!glfwInit()) {
		die("could not init glfw\n");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	auto window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		die("could not create window\n");
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		die("could not init glad\n");
	}

	glfwSetKeyCallback(window, on_key_input);
	glfwSetMouseButtonCallback(window, on_mouse_button);
	glfwSetFramebufferSizeCallback(window, on_window_resize);

	if (glfwRawMouseMotionSupported()) {
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, true);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

void draw_gui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Press G to toggle this window");

	ImGui::ColorEdit3("Ambient Light", G.color_renderer.ambient_light);
	ImGui::ColorEdit3("Diffuse Light", G.color_renderer.diffuse_light);
	ImGui::Separator(); // TODO remove

	if (G.selected_object) {
		ImGui::ColorEdit3("Object Color", G.selected_object->color);
	}

	if (!ImGui::CollapsingHeader("Shadows")) {
		auto algorithm = (int*)&G.color_renderer.algorithm;
		ImGui::Combo("Algorithm", algorithm, "Hard Shadows\0PCF\0PCSS\0");

		if (*algorithm == G.color_renderer.PCF) {
			ImGui::SliderFloat("Radius", &G.color_renderer.filter_radius, 0.0, 0.05);
			// ImGui::DragFloat("Radius", &G.color_renderer.filter_radius, 0.0001, 0.0, 0.1);
		}
		else if (*algorithm == G.color_renderer.PCSS) {
			// TODO bug this slider goes negative
			ImGui::DragFloat("Light Width", &G.color_renderer.light_width, 0.005, 0.0f);
		}

		if (*algorithm != G.color_renderer.HARD_SHADOWS) {
			auto pattern = (int*)&G.color_renderer.filter_pattern;
			ImGui::Combo("Pattern", pattern, "Regular Grid\0Poisson Disk\0Rotated Poisson\0");

			// TODO regular sampling / poisson sampling
			// TODO slider goes negative (because no maximum value is given)
			// TODO max value !
			ImGui::DragInt("Sample Count", &G.color_renderer.sample_count, 0.1, 1);
		}
	}

	if (ImGui::CollapsingHeader("Camera")) {
		bool is_primary_cam = G.active_camera == &G.primary_camera;
		if (ImGui::RadioButton("Primary Camera", is_primary_cam)) {
			G.active_camera = &G.primary_camera;
		}
		if (ImGui::RadioButton("Light Source Camera", !is_primary_cam)) {
			G.active_camera = &G.shadow_camera;
		}

		bool changed = false;
		changed |= ImGui::SliderFloat("FOV", &G.active_camera->fov_y, 0.0f, 3.14f);
		// changed |= ImGui::SliderFloat("Near Plane", &G.active_camera->near_plane, 0.1f, );
		// changed |= ImGui::SliderFloat("Far Plane", &G.active_camera->far_plane, 0.0f, 3.14f);
		if (changed) {
			// TODO aspect is needed !
			// G.active_camera->calc_projection(float aspect)
		}
	}

	ImGui::End();

	if (G.selected_object) {
		auto const *view = (float*)G.active_camera->view_matrix;
		auto const *proj = (float*)G.active_camera->proj_matrix;
		auto *model = (float*)G.selected_object->model_matrix;
		auto mode = ImGuizmo::LOCAL;
		if (G.guizmo_op == ImGuizmo::TRANSLATE) {
			mode = ImGuizmo::WORLD;
		}

		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(0, 0, G.default_fbo.width, G.default_fbo.height);
		ImGuizmo::Manipulate(view, proj, G.guizmo_op, mode, model);
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void apply_camera_movement(camera &cam) {
	static double prevX;
	static double prevY;

	double mouseX;
	double mouseY;
	glfwGetCursorPos(G.window, &mouseX, &mouseY);

	if (glfwGetMouseButton(G.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		vec2 delta {
			float(prevX - mouseX) * MOUSE_ACCEL,
			float(prevY - mouseY) * MOUSE_ACCEL
		};
		vec2_add(cam.rotation, cam.rotation, delta);
	}
	prevX = mouseX;
	prevY = mouseY;

	// TODO WantCaptureMouse WantCaptureKeyboard

	const struct {
		int key;
		vec3 dir;
	} bindings[] = {
		{GLFW_KEY_W,			{ 0,  0,  1}},
		{GLFW_KEY_A,			{-1,  0,  0}},
		{GLFW_KEY_S,			{ 0,  0, -1}},
		{GLFW_KEY_D,			{ 1,  0,  0}},
		{GLFW_KEY_SPACE,		{ 0,  1,  0}},
		{GLFW_KEY_LEFT_CONTROL, { 0, -1,  0}},
	};

	vec3 local_movement {0.0f, 0.0f, 0.0f};
	for (auto b : bindings) {
		if (glfwGetKey(G.window, b.key) == GLFW_PRESS) {
			vec3 vec;
			vec3_scale(vec, b.dir, MOVEMENT_SPEED);
			vec3_add(local_movement, local_movement, vec);
		}
	}

	float c = cosf(-cam.rotation[0]);
	float s = sinf(-cam.rotation[0]);
	vec3_add(cam.position, cam.position, vec3 {
		c * local_movement[0] + s * local_movement[2],
		local_movement[1],
		s * local_movement[0] - c * local_movement[2]
	});
	
	cam.calc_view();
}

int main() {
	const int win_width = 850;
	const int win_height = 650;
	G.window = initialize(win_width, win_height, "VWA Demo");

	G.objects = {
		mesh::load_stl("models/things.stl", vec3{0.5, 0.0, 0.0}),
		mesh::load_stl("models/plane.stl", vec3{0.0, 0.0, 0.0})
	};

	G.default_fbo = framebuffer(win_width, win_height);
	G.primary_camera = camera(vec3{-1.5, 0.4, -4.2}, vec3{0.0, -10.0, 0.0});
	// TODO
	G.primary_camera.far_plane = 40.0;
	G.primary_camera.calc_projection(G.default_fbo.get_aspect_ratio());

	auto shadow_fbo = framebuffer::make_shadow_buffer(1024, 1024);
	G.shadow_camera = camera(vec3{0.0, 13.0, -7.0}, vec3{0.0, 0.0, 0.0});
	// TODO
	G.shadow_camera.near_plane = 5.0;
	G.shadow_camera.far_plane = 25.0;
	G.shadow_camera.calc_projection(shadow_fbo.get_aspect_ratio());

	program color_prog("shaders/default.vs", "shaders/default.fs");
	program shadow_prog("shaders/shadow.vs", "shaders/shadow.fs");
	G.color_renderer.shadow_camera = &G.shadow_camera;
	G.color_renderer.shadow_fbo = &shadow_fbo;
	
	while (!glfwWindowShouldClose(G.window)) {
		glfwPollEvents();
		apply_camera_movement(*G.active_camera);

		shadow_fbo.bind_for_rendering();
		G.shadow_renderer.draw(shadow_prog, G.shadow_camera, G.objects);

		G.default_fbo.bind_for_rendering();
		G.color_renderer.draw(color_prog, *G.active_camera, G.objects);

		if (G.show_gui) {
			draw_gui();
		}

		glfwSwapBuffers(G.window);
	}
}