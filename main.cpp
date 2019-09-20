#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <map>
#include <iostream>

#include <core/scene.h>
#include <core/shader.h>
#include <core/camera.h>
#include <core/serialize.h>
#include <core/camera_animator.h>

void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
constexpr auto screen_width = 640;
constexpr auto screen_height = 480;

// camera
Camera &get_camera() {
    static Camera camera;
    return camera;
}

float lastX = (float)screen_width;
float lastY = (float)screen_height / 2.0;
bool firstMouse = true;
bool camera_animation_enabled = true;

// timing
float deltaTime = 0.0f;
float last_frame_time = 0.0f;

// Camera settings
constexpr auto near_plane = 0.01f;
constexpr auto fov = 60.0f;

int main(int argc, char *argv[]) {
    
    std::string scene_path{"data/scenes/sun_temple/SunTemple.scene"};
    if (argc > 1) {
        scene_path = argv[1];
    }
    
    std::cout << "Loading scene: " << scene_path << std::endl;
    auto scene = SceneInfo::load(scene_path);
    scene.print();
    
    if (!scene.cameras().empty()) {
        auto &&camera_state = scene.cameras().front();
        auto camera_dir = glm::normalize(camera_state.lookat - camera_state.eye);
        get_camera() = Camera{camera_state.eye, camera_state.up};
    }
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_SRGB_CAPABLE, 1);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif
    
    // glfw window creation
    // ====================
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow *window = glfwCreateWindow(screen_width, screen_height, "LuisaVR", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // tell GLFW to capture our mouse
    if (!camera_animation_enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    
    // glad: load all OpenGL function pointers
    // =======================================
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // create scene
    auto geometry = Geometry::create(scene);
    auto far_plane = glm::length(geometry.aabb().max - geometry.aabb().min) * 1.1f;
    
    // build and compile shaders
    Shader shader{"data/shaders/ggx.vs", "data/shaders/ggx_approx.fs", {}, {
        {std::string{"LIGHT_COUNT"}, serialize(scene.lights().size())},
        {std::string{"TEXTURE_MAX_SIZE"}, serialize(4096)}}};
    
    auto animation_time = 0.0f;
    auto camera_animator = CameraAnimator::create(scene);
    
    double last_fps_time = glfwGetTime();
    int nbFrames = 0;
    
    auto count = 0;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);
    
    while (!glfwWindowShouldClose(window)) {
        
        auto current_time = static_cast<float>(glfwGetTime());
        animation_time = static_cast<float>(current_time);
        
        nbFrames++;
        if (nbFrames == 30) {
            std::cout << "FPS: " << nbFrames / (current_time - last_fps_time) << std::endl;
            last_fps_time = current_time;
            nbFrames = 0;
        }
        
        deltaTime = current_time - last_frame_time;
        last_frame_time = current_time;
        
        processInput(window);
        
        auto view_matrix = get_camera().GetViewMatrix();
        auto camera_position = get_camera().GetPosition();
        if (camera_animation_enabled && !camera_animator.empty()) {
            auto state = camera_animator.state(animation_time);
            camera_position = state.eye;
            view_matrix = glm::lookAt(state.eye, state.lookat, state.up);
        }
        
        auto frame_width = 0;
        auto frame_height = 0;
        glfwGetFramebufferSize(window, &frame_width, &frame_height);
        
        auto projection = glm::perspective(glm::radians(fov), static_cast<float>(frame_width) / static_cast<float>(frame_height), near_plane, far_plane);
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(static_cast<uint32_t>(GL_COLOR_BUFFER_BIT) | static_cast<uint32_t>(GL_DEPTH_BUFFER_BIT));
        glViewport(0, 0, frame_width, frame_height);
        shader.use();
        for (auto i = 0ul; i < scene.lights().size(); i++) {
            shader.setVec3(serialize("lights[", i, "].Position"), scene.lights()[i].position);
            shader.setVec3(serialize("lights[", i, "].Color"), scene.lights()[i].emission);
        }
        shader.setMat4("projection", projection);
        shader.setMat4("view", view_matrix);
        shader.setVec3("cameraPos", camera_position);
        geometry.render(shader);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        count++;
    }
    
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (!camera_animation_enabled) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            get_camera().ProcessKeyboard(FORWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            get_camera().ProcessKeyboard(BACKWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            get_camera().ProcessKeyboard(LEFT, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            get_camera().ProcessKeyboard(RIGHT, deltaTime);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        get_camera().SetWeight(1.0);
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        get_camera().SetWeight(0.0);
    }
    
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        camera_animation_enabled = true;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        camera_animation_enabled = false;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *, double xpos, double ypos) {
    
    if (!camera_animation_enabled) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
        
        lastX = xpos;
        lastY = ypos;
        
        get_camera().ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *, double, double yoffset) {
    get_camera().ProcessMouseScroll(yoffset);
}
