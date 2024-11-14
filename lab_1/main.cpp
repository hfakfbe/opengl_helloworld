#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

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

// transformation
glm::vec3 modelTranslation(0.0f, 0.0f, 0.0f);  // 模型的平移
glm::vec3 modelRotation(0.0f, 0.0f, 0.0f);        // 模型的旋转角度（绕Y轴旋转）
float modelScale = 1.0f;                          // 模型的缩放系数

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
    // glfwSetCursorPosCallback(window, mouse_callback);
    // glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

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

    
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model, dispose transformations
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, modelTranslation);                          // 平移
        model = glm::rotate(model, glm::radians(modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // 绕Y轴旋转
        model = glm::rotate(model, glm::radians(modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // 绕Y轴旋转
        model = glm::rotate(model, glm::radians(modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // 绕Y轴旋转
        model = glm::scale(model, glm::vec3(modelScale));  
        
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);


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

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // 平移控制
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        modelTranslation.z += 0.005f; // 前平移
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        modelTranslation.z -= 0.005f; // 后平移
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        modelTranslation.y += 0.005f; // 上平移
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        modelTranslation.y -= 0.005f; // 下平移
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        modelTranslation.x -= 0.005f; // 左平移
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        modelTranslation.x += 0.005f; // 右平移

    // 旋转控制
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        modelRotation.x += 0.1f; // x轴旋转
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
        modelRotation.z += 0.1f; // z轴旋转
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        modelRotation.y += 0.1f; // y轴顺时针旋转
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        modelRotation.y -= 0.1f; // y轴逆时针旋转

    // 缩放控制
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        modelScale += 0.0005f; // 放大
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        modelScale -= 0.0005f; // 缩小
    modelScale = std::max(0.1f, modelScale); // 限制最小缩放为0.1，防止变为0或负值

    std::cout << "modelTranslation: (" << modelTranslation.x << ", " << modelTranslation.y << ", " << modelTranslation.z << ")" << "\n";
    std::cout << "modelRotation: (" << modelRotation.x << ", " << modelRotation.y << ", " << modelRotation.z << ")" << "\n";
    std::cout << "modelScale: " << modelScale << "\n";
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}