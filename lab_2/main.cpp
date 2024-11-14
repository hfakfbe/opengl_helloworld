#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void resetParameters();

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// camera
Camera camera(glm::vec3(0.0f, 4.5f, 12.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 定义初始和终止位置以及四元数旋转变量
glm::vec3 startPosition;    // 起始位置
glm::vec3 endPosition;      // 终止位置
glm::quat startRotation;    // 起始旋转（四元数）
glm::quat endRotation;      // 终止旋转（四元数）
float transitionDuration; // 过渡的总时间（秒）
float transitionTime;     // 当前的过渡时间                   // 模型的缩放系数
bool isTransitioning;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "图形学实验", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 初始化 Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 24.0f); // 修改为字体路径和所需字号
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("shader.vs", "shader.fs");

    // load models
    // -----------
    Model ourModel("../model/tower/wooden watch tower2.obj");

    resetParameters();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // 启动 ImGui 新一帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 设置 ImGui 控制面板
        ImGui::Begin("Control Panel");
        ImGui::InputFloat3("Start Position", &startPosition.x, "%.3f"); // "%.3f" 控制显示精度
        ImGui::InputFloat3("End Position", &endPosition.x, "%.3f"); // "%.3f" 控制显示精度
        ImGui::SliderFloat("Transition Time", &transitionDuration, 0.1f, 10.0f);
        // 起始和终止姿态控制（以欧拉角表示，之后转化为四元数）
        static glm::vec3 startEuler(0.0f, 0.0f, 0.0f);
        static glm::vec3 endEuler(glm::radians(90.0f), glm::radians(45.0f), glm::radians(30.0f));
        ImGui::InputFloat3("Begin Euler", &startEuler.x, "%.3f"); // "%.3f" 控制显示精度
        ImGui::InputFloat3("End Euler", &endEuler.x, "%.3f"); // "%.3f" 控制显示精度
        startRotation = glm::quat(startEuler);
        endRotation = glm::quat(endEuler);
        // 控制开始和重置按钮
        if (ImGui::Button("Start")){
            transitionTime = 0.0f;
            isTransitioning = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
            resetParameters();
        ImGui::End();

        // 渲染模型
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        if (isTransitioning){
            transitionTime += deltaTime;
        }
        // 计算插值
        float t = transitionTime / transitionDuration;
        t = glm::clamp(t, 0.0f, 1.0f);
        glm::vec3 currentPosition = glm::mix(startPosition, endPosition, t);
        glm::quat currentRotation = glm::slerp(startRotation, endRotation, t);
        // 创建模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, currentPosition);
        model *= glm::toMat4(currentRotation);
        ourShader.use();
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);
        if (t >= 1.0f){
            isTransitioning = false;
        }

        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void resetParameters() {
    startPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    endPosition = glm::vec3(3.0f, 3.0f, 3.0f);
    startRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
    endRotation = glm::quat(glm::vec3(glm::radians(90.0f), glm::radians(45.0f), glm::radians(30.0f)));
    transitionDuration = 5.0f;
    transitionTime = 0.0f;
    isTransitioning = false;
}