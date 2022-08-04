#include <glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <cstdlib>
#include "program.hh"
#include "mesh.hh"
#include "camera.hh"
#include "obj_parser/parser.hh"
#include "shadowmap.hh"

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
	int winWidth {1260};
	int winHeight {750};
	Context window {winWidth, winHeight, "Shadow Mapping"};

	Camera camera {glm::vec3(9.0f, 9.5f, 8.5f), glm::vec3(0.0f)};
	Camera *activeCamera {&camera};
	Program normalPass {"source/shaders/normalPass.vert", "source/shaders/normalPass.frag"};
	std::vector<Mesh> meshes {loadMeshesFromFile("assets/final.obj")};

	ShadowMap shadowMap {glm::vec3(-8.0f, 15.0f, 10.0f), glm::vec3(0.0f)};

	int panelWidth {320};
	bool animateLight {false};
	bool lookAround {false};

	int shadowQuality {3};
	float filterRadius {0.007f};
	bool enablePCSS {true};
	float lightWidth {0.65f};
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

		// Sets the correct projection matrix.
		onFramebufferResize(window, winWidth, winHeight);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiStyle &style = ImGui::GetStyle();
		style.Colors[ImGuiCol_Border].w = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330 core");
	}

	void enterMainLoop()
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		double prevTime = glfwGetTime();
		while (!glfwWindowShouldClose(window)) {
			double time = glfwGetTime();
			auto deltaTime = (float) (time - prevTime);
			prevTime = time;

			if (animateLight) {
				shadowMap.getCamera().animate(deltaTime);
			}

			glfwPollEvents();
			handleUserInput(deltaTime);

			shadowMap.renderShadowPass(meshes);
			renderNormalPass();
			renderGui(deltaTime);
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
		if (lookAround && isMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
			mouseDelta.x = float(mouseX - prevMouseX);
			mouseDelta.y = float(mouseY - prevMouseY);
		}
		prevMouseX = mouseX;
		prevMouseY = mouseY;

		if (glm::length(displacement) > 0.0f) {
			displacement = glm::normalize(displacement);
		}
		displacement *= SPEED * deltaTime;
		mouseDelta *= SENSITIVITY * deltaTime;
		activeCamera->applyUserInput(displacement, mouseDelta);
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
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, winWidth - panelWidth, winHeight);
		glClearColor(0.53f, 0.58f, 0.66f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);

		Camera const &shadowCamera = shadowMap.getCamera();
		GLuint depthAttachment = shadowMap.getDepthAttachment();
		normalPass.use();
		normalPass.set("uView", activeCamera->viewMatrix);
		normalPass.set("uProj", activeCamera->projMatrix);
		normalPass.set("uLightView", shadowCamera.viewMatrix);
		normalPass.set("uLightProj", shadowCamera.projMatrix);
		normalPass.set("uLightPosition", shadowCamera.position);
		normalPass.set("uLightNearPlane", shadowCamera.nearPlane);
		normalPass.set("uLightFarPlane", shadowCamera.farPlane);
		normalPass.set("uLightWidthUV", lightWidth / shadowCamera.getFrustumWidth());
		normalPass.setTexture("uShadowSampler", 0, depthAttachment);
		normalPass.setTexture("uDepthBuffer", 0, depthAttachment);
		normalPass.set("uShadowQuality", shadowQuality);
		normalPass.set("uFilterRadius", filterRadius);
		normalPass.set("uEnablePCSS", enablePCSS);

		for (auto const &mesh: meshes) {
			normalPass.set("uModel", mesh.getModelMatrix());
			mesh.drawArrays();
		}
	}

	void renderGui(float deltaTime)
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos({float(winWidth - panelWidth), 0});
		ImGui::SetNextWindowSize({float(panelWidth), float(winHeight)});
		if (ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_NoDecoration)) {
			renderGuiItems(deltaTime);
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void renderGuiItems(float deltaTime)
	{
		ImGui::SliderInt("Quality", &shadowQuality, 0, 3);
		if (ImGui::RadioButton("PCF", !enablePCSS)) {
			enablePCSS = false;
		}
		if (ImGui::RadioButton("PCSS", enablePCSS)) {
			enablePCSS = true;
		}
		if (enablePCSS) {
			ImGui::SliderFloat("Light width", &lightWidth, 0.1f, 1.4f);
		} else {
			ImGui::SliderFloat("Filter radius", &filterRadius, 0.0f, 0.03f);
		}

		ImGui::Separator();

		bool isPrimary = activeCamera == &camera;
		if (ImGui::RadioButton("Primary camera", isPrimary)) {
			activeCamera = &camera;
		}
		if (ImGui::RadioButton("Shadow camera", !isPrimary)) {
			activeCamera = &shadowMap.getCamera();
		}
		bool changed = false;
		changed |= ImGui::SliderFloat("FOV", &activeCamera->fovY, 0.1f, 3.14f);
		changed |= ImGui::SliderFloat("Near plane", &activeCamera->nearPlane, 0.1f, 10.0f);
		changed |= ImGui::SliderFloat("Far plane", &activeCamera->farPlane, 20.0f, 100.0f);
		if (changed) {
			activeCamera->updateProjectionMatrix();
		}
		ImGui::Checkbox("Animate light", &animateLight);

		ImGui::Separator();

		ImGui::Text("FPS: %d", int(1.0f / deltaTime));
		ImGui::Text("Delta time: %f ms", deltaTime * 1000);
	}

	static void onKeyInput(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
	}

	static void onMouseButton(GLFWwindow *window, int button, int action, int mods)
	{
		Application *app = getApplication(window);
		if (action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
			if (button == GLFW_MOUSE_BUTTON_RIGHT) {
				app->lookAround = true;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
		}
		if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT) {
			app->lookAround = false;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	static void onFramebufferResize(GLFWwindow *window, int width, int height)
	{
		Application *app = getApplication(window);
		app->winWidth = width;
		app->winHeight = height;
		app->camera.setAspectRatio(width - app->panelWidth, height);
	}

	[[nodiscard]] static Application *getApplication(GLFWwindow *window)
	{
		return static_cast<Application *>(glfwGetWindowUserPointer(window));
	}
};

int main()
{
	try {
		Application app;
		app.enterMainLoop();
	} catch (std::string &message) {
		std::cerr << message << '\n';
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
