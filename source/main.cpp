#include <glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "program.hh"
#include "mesh.hh"
#include "camera.hh"
#include "obj_parser/parser.hh"

void onGlfwError(int code, char const *description)
{
	(void) code;
	std::cerr << "GLFW error: " << description << '\n';
}

class Context {
	GLFWwindow *window {nullptr};
public:
	Context(int width, int height, char const *title)
	{
		glfwSetErrorCallback(onGlfwError);
		if (!glfwInit()) {
			util::fatalError("Could not init glfw");
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (!window) {
			glfwTerminate();
			util::fatalError("Could not create window");
		}

		glfwMakeContextCurrent(window);
		if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
			glfwTerminate();
			util::fatalError("Could not load OpenGL functions");
		}
	}

	Context(Context const &) = delete;

	Context &operator=(Context const &) = delete;

	~Context()
	{
		glfwTerminate();
	}

	[[nodiscard]] operator GLFWwindow *() const
	{
		return window;
	}
};

class Application {
	int width {640};
	int height {480};
	Context window {width, height, "Shadow Mapping"};

	Camera camera {glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f)};
	Program normalPass {"shaders/normalPass.vert", "shaders/normalPass.frag"};

	// TODO path
	std::vector<Mesh> meshes {loadMeshesFromFile("/home/andrey/NaturePack/untitled.obj")};
public:
	Application()
	{
		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, onKeyInput);
		glfwSetMouseButtonCallback(window, onMouseButton);
		glfwSetFramebufferSizeCallback(window, onFramebufferResize);

		if (glfwRawMouseMotionSupported()) {
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, true);
		}

		camera.setAspectRatio(width, height);

		glEnable(GL_DEPTH_TEST);
		// TODO
		glEnable(GL_CULL_FACE);
	}

	void enterMainLoop()
	{
		double prevTime = glfwGetTime();
		while (!glfwWindowShouldClose(window)) {
			double time = glfwGetTime();
			double deltaTime = time - prevTime;
			prevTime = time;

			glfwPollEvents();
			handleUserInput((float) deltaTime);

			renderNormalPass();
			glfwSwapBuffers(window);
		}
	}

private:
	void handleUserInput(float deltaTime)
	{
		float const SPEED = 14.0f;
		float const SENSITIVITY = 1.4f;
		glm::vec3 displacement(0.0f);
		glm::vec2 mouseDelta(0.0f);

		if (isPressed(GLFW_KEY_W)) {
			displacement += glm::vec3(0.0f, 0.0f, -1.0f);
		}
		if (isPressed(GLFW_KEY_A)) {
			displacement += glm::vec3(-1.0f, 0.0f, 0.0f);
		}
		if (isPressed(GLFW_KEY_S)) {
			displacement += glm::vec3(0.0f, 0.0f, 1.0f);
		}
		if (isPressed(GLFW_KEY_D)) {
			displacement += glm::vec3(1.0f, 0.0f, 0.0f);
		}
		if (isPressed(GLFW_KEY_SPACE)) {
			displacement += glm::vec3(0.0f, 1.0f, 0.0f);
		}
		if (isPressed(GLFW_KEY_LEFT_CONTROL)) {
			displacement += glm::vec3(0.0f, -1.0f, 0.0f);
		}

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		static double prevMouseX = mouseX;
		static double prevMouseY = mouseY;
		if (isMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
			mouseDelta.x = float(mouseX - prevMouseX);
			mouseDelta.y = float(mouseY - prevMouseY);
		}
		prevMouseX = mouseX;
		prevMouseY = mouseY;

		if (glm::length(displacement) > 0.0f) {
			displacement = glm::normalize(displacement);
		}
		camera.applyUserInput(displacement * SPEED * deltaTime,
		                      mouseDelta * SENSITIVITY * deltaTime);
	}

	[[nodiscard]] bool isPressed(int key) const
	{
		return glfwGetKey(window, key) == GLFW_PRESS;
	}

	[[nodiscard]] bool isMouseDown(int button) const
	{
		return glfwGetMouseButton(window, button) == GLFW_PRESS;
	}

	void renderNormalPass() const
	{
		glViewport(0, 0, width, height);
		// TODO
		double f = 1.3;
		glClearColor(0.62 / f, 0.75 / f, 0.78 / f, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		normalPass.use();
		normalPass.set("uView", camera.viewMatrix);
		normalPass.set("uProj", camera.projMatrix);

		for (Mesh const &mesh: meshes) {
			normalPass.set("uModel", mesh.modelMatrix);
			glBindVertexArray(mesh.vao);
			glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
		}

	}

	static void onKeyInput(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		// TODO
	}

	static void onMouseButton(GLFWwindow *window, int button, int action, int mods)
	{
		if (action == GLFW_PRESS) {
			switch (button) {
			case GLFW_MOUSE_BUTTON_RIGHT:
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				break;
			}
		}
		if (action == GLFW_RELEASE) {
			switch (button) {
			case GLFW_MOUSE_BUTTON_RIGHT:
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				break;
			}
		}
	}

	static void onFramebufferResize(GLFWwindow *window, int width, int height)
	{
		Application *app = getApplication(window);
		app->width = width;
		app->height = height;
		app->camera.setAspectRatio(width, height);
	}

	[[nodiscard]] static Application *getApplication(GLFWwindow *window)
	{
		return static_cast<Application *>(glfwGetWindowUserPointer(window));
	}
};

void HandleUserInput(GLFWwindow *mWindow, Camera &mCamera)
{
	static glm::vec2 prevMousePos = mCamera.position;
	float const SPEED = 15.5;
	float const SENSITIVITY = 2.0;
	float deltaTime = 0.01;

	struct {
		int key;
		glm::vec3 dir;
	} bindings[] = {
		{GLFW_KEY_W,            {0,  0,  -1}},
		{GLFW_KEY_A,            {-1, 0,  0}},
		{GLFW_KEY_S,            {0,  0,  1}},
		{GLFW_KEY_D,            {1,  0,  0}},
		{GLFW_KEY_SPACE,        {0,  1,  0}},
		{GLFW_KEY_LEFT_CONTROL, {0,  -1, 0}},
	};

	glm::vec3 displacement {0, 0, 0};
	for (auto &b: bindings) {
		if (glfwGetKey(mWindow, b.key) == GLFW_PRESS) {
			displacement += b.dir * SPEED * deltaTime;
		}
	}

	double mouseX;
	double mouseY;
	glfwGetCursorPos(mWindow, &mouseX, &mouseY);
	glm::vec2 mouse {mouseX, mouseY};
	glm::vec2 deltaMouse {0, 0};
	if (glfwGetMouseButton(mWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		deltaMouse = (mouse - prevMousePos) * SENSITIVITY * deltaTime;
	}
	prevMousePos = mouse;

	mCamera.applyUserInput(displacement, deltaMouse);
}

void DrawGUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuiWindowFlags flags = 0;
	flags |= ImGuiWindowFlags_NoDecoration;
	bool show = true;
	ImGui::Begin("##Options", &show, flags);
	{
		ImGui::Button("My cool Button");
	}
	ImGui::End();
	// if (!show) {
	// 	glfwSetWindowShouldClose(window, true);
	// }

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO &io = ImGui::GetIO();
	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow *backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

int main()
{
	try {
		Application app;
		app.enterMainLoop();
	} catch (std::string &message) {
		std::cerr << message << '\n';
		return 1;
	}
	return 0;

	// TODO: Do not forget to remove this.
	// IMGUI_CHECKVERSION();
	// ImGui::CreateContext();
	// ImGuiIO &io = ImGui::GetIO();
	// (void) io;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	// io.ConfigFlags |= ImGuiViewportFlags_TopMost;
	// io.ConfigViewportsNoAutoMerge = true;
	// io.ConfigViewportsNoTaskBarIcon = true;

	// io.ConfigFlags |= ImGuiConfigFl
	// io.ConfigWindowsMoveFromTitleBarOnly = true;

	// io.ConfigWindowsResizeFromEdges = false;

	// io.ConfigViewportsNoDecoration = false;

	// io.ConfigFlags |= ImGuiViewportFlags_NoDecoration;
	// ImGuiViewport::Flags |= ImGuiViewportFlags_NoDecoration;

	// ImGui::StyleColorsClassic();

	// ImGuiStyle &style = ImGui::GetStyle();
	// if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
	// 	// style.WindowRounding = 0.0f;
	// 	// style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	// 	style.Colors[ImGuiCol_Border].w = 0.0f;
	// }

	// ImGui_ImplGlfw_InitForOpenGL(window, true);
	// ImGui_ImplOpenGL3_Init("#version 330 core");
}
