#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "shader.h"
#include "object.h"

#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <functional>

int SCR_WIDTH = 1200, SCR_HEIGHT = 800;
std::vector<unsigned char> pixelBuffer;
unsigned int VAO, VBO, texture;

Sphere redSphere({-1.0f, -1.0f, -4.0f}, 1.0f, {1.0f, 0.0f, 0.0f}, 0.2f);
Sphere blueSphere({1.0f, -1.0f, -4.0f}, 1.0f, {0.0f, 0.0f, 1.0f}, 0.2f);

Wall floors({0.0f, -2.0f, -3.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 20.0f, 20.0f, {0.5f, 0.3f, 0.1f}, 0.1f);
Wall leftWall({-2.0f, 0.0f, -3.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 20.0f, 20.0f, {1.0f, 1.0f, 1.0f}, 0.01f);
Wall backWall({0.0f, 0.0f, -5.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, 20.0f, 20.0f, {1.0f, 1.0f, 1.0f}, 0.01f);

std::vector<Object*> objects = {&redSphere, &blueSphere, &floors, &leftWall, &backWall};

glm::vec3 trace(const Ray& ray, const std::vector<Object*>& objects, const Light& light, int depth) {
    if (depth > 3) return glm::vec3(0.0f); // 终止条件

    float t_min = std::numeric_limits<float>::max();
    const Object* hitObject = nullptr;
    glm::vec3 hitNormal;

    // 遍历所有物体，找到最近的交点
    for (const auto* object : objects) {
        float t;
        glm::vec3 normal;
        if (object->intersect(ray, t, normal) && t > 0.001f && t < t_min) {
            t_min = t;
            hitObject = object;
            hitNormal = normal;
        }
    }

    if (!hitObject) return glm::vec3(0.0f, 0.0f, 0.0f); // 背景颜色

    glm::vec3 hitPoint = ray.origin + t_min * ray.direction;
    glm::vec3 lightDir = glm::normalize(light.position - hitPoint);

    // 检测阴影：检查光源到交点之间是否有阻挡
    bool inShadow = false;
    Ray shadowRay;
    shadowRay.origin = hitPoint + hitNormal * 0.001f; // 偏移以避免浮点精度问题
    shadowRay.direction = lightDir;

    for (const auto* object : objects) {
        float t;
        glm::vec3 normal;
        if (object->intersect(shadowRay, t, normal) && t > 0.001f && t <= glm::length(light.position - hitPoint)) {
            inShadow = true;
            break;
        }
    }

    // 如果在阴影中，将漫反射和镜面反射光照设置为0
    glm::vec3 diffuse(0.0f);
    glm::vec3 specular(0.0f);
    if (!inShadow) {
        // 计算漫反射
        float diff = glm::max(glm::dot(hitNormal, lightDir), 0.0f);
        diffuse = diff * hitObject->color * light.color;

        // 计算镜面反射
        glm::vec3 viewDir = glm::normalize(-ray.direction);
        glm::vec3 reflectDir = glm::reflect(-lightDir, hitNormal);
        float spec = glm::pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), 32);
        specular = spec * light.color;
    }

    // 递归反射
    glm::vec3 reflectionColor(0.0f);
    if (hitObject->reflectivity > 0.0f) {
        Ray reflectedRay;
        reflectedRay.origin = hitPoint + hitNormal * 0.001f; // 避免浮点精度问题
        reflectedRay.direction = glm::reflect(ray.direction, hitNormal);
        reflectionColor = trace(reflectedRay, objects, light, depth + 1);
    }

    return diffuse + specular + reflectionColor * hitObject->reflectivity;
}

// 渲染单行的函数
void renderRow(int startY, int endY, int width, int height, const Camera& camera, const Light& light) {
    float aspectRatio = float(width) / float(height);
    float scale = glm::tan(glm::radians(camera.fov * 0.5f));

    // 计算前方向、右方向、上方向
    glm::vec3 forward = glm::normalize(camera.direction);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(camera.angle), forward);
    right = glm::vec3(rotation * glm::vec4(right, 1.0f));
    up = glm::vec3(rotation * glm::vec4(up, 1.0f));

    // 渲染每一行
    for (int y = startY; y < endY; ++y) {
        for (int x = 0; x < width; ++x) {
            float px = (2 * (x + 0.5f) / float(width) - 1) * aspectRatio * scale;
            float py = (2 * (y + 0.5f) / float(height) - 1) * scale;

            glm::vec3 dir = glm::normalize(forward + px * right + py * up);
            Ray ray = {camera.position, dir};

            glm::vec3 color = trace(ray, objects, light, 0);

            // 计算像素索引并保护对 pixelBuffer 的写操作
            int index = (y * width + x) * 3;
            {
                // std::lock_guard<std::mutex> lock(bufferMutex);
                pixelBuffer[index] = static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255);
                pixelBuffer[index + 1] = static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255);
                pixelBuffer[index + 2] = static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255);
            }
        }
    }
}

// 多线程渲染函数
void renderScene(std::vector<unsigned char>& pixelBuffer, int width, int height, const Camera& camera, const Light& light) {
    int numThreads = std::thread::hardware_concurrency();
    int rowsPerThread = height / numThreads;

    std::vector<std::thread> threads;
    // std::mutex bufferMutex;

    for (int i = 0; i < numThreads; ++i) {
        int startY = i * rowsPerThread;
        int endY = (i == numThreads - 1) ? height : startY + rowsPerThread;

        threads.emplace_back(renderRow, startY, endY, width, height, std::ref(camera), std::ref(light));
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
}

void initTexture(unsigned int &texture, std::vector<unsigned char> &pixelBuffer, int SCR_WIDTH, int SCR_HEIGHT) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void initQuad(unsigned int &VAO, unsigned int &VBO) {
    float vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGuiIO& io = ImGui::GetIO(); 
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 24.0f); // 修改为字体路径和所需字号
    ImGui_ImplOpenGL3_Init("#version 330");
}

bool makeImGui(Light& light, Camera& camera) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Control Panel");

    ImGui::SliderFloat3("Light Position", &light.position[0], -10.0f, 10.0f);
    ImGui::ColorEdit3("Light Color", &light.color[0]);

    ImGui::SliderFloat3("Camera Position", &camera.position[0], -10.0f, 10.0f);
    ImGui::SliderFloat3("Camera Direction", &camera.direction[0], -1.0f, 1.0f); // 方向调整
    ImGui::SliderFloat("Camera Angle", &camera.angle, -180.0f, 180.0f);         // 角度调整
    ImGui::SliderFloat("FOV", &camera.fov, 10.0f, 120.0f);

    // bool startRender = false;
    // if (ImGui::Button("Start")) {
    //     startRender = true;
    // }
    ImGui::End();

    return true; // startRender;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    width += 4 - width % 4;
    SCR_HEIGHT = height;
    SCR_WIDTH = width;
    pixelBuffer.resize(width * height * 3);
    glViewport(0, 0, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.data());
    std::cout << "width: " << width << ", height: " << height << std::endl;
}

int main() {
    // 初始化GLFW
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Ray Tracing", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 创建着色器
    Shader ourShader("shader.vs", "shader.fs");
    
    // 初始化渲染
    initQuad(VAO, VBO);
    initTexture(texture, pixelBuffer, SCR_WIDTH, SCR_HEIGHT);
    framebuffer_size_callback(window, SCR_WIDTH, SCR_HEIGHT);

    // 初始化 Dear ImGui
    initImGui(window);

    // 可调参数
    Light light = {{5.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}};
    Camera camera = {
        .position = glm::vec3(0.0f, 0.0f, 0.0f),
        .direction = glm::vec3(0.0f, 0.0f, -1.0f),
        .angle = 0.0f,
        .fov = 90.0f
    };

    // 进入渲染循环
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (makeImGui(light, camera)) {
            // 渲染
            float beg = glfwGetTime();
            renderScene(pixelBuffer, SCR_WIDTH, SCR_HEIGHT, camera, light);
            std::cout << "Render time: " << glfwGetTime() - beg << "\n";

            // 更新纹理数据
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.data());
        }

        // 绘制纹理到屏幕
        glClear(GL_COLOR_BUFFER_BIT);
        ourShader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

