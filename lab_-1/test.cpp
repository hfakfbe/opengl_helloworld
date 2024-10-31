#include "../GLFW/glfw3.h"
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;

int length = 1200, height = 800, pxSize = 20;
int numL = length / pxSize, numH = height / pxSize;
double deltaL = 2.0 * pxSize / (length * 1.0);
double deltaH = 2.0 * pxSize / (height * 1.0);
struct COLOR {
    double R, G, B;
    void init(double r, double g, double b) {
        R = r, G = g, B = b;
    }
} col[15];

void initColor() {
    col[0].init(0.1, 0.1, 0.1);
    col[1].init(0.0, 0.75, 1);
    col[2].init(0.196, 0.80, 0.196);
    col[3].init(1.0, 0.84, 0.0);
    col[4].init(0.98, 0.50, 0.45);
    col[5].init(0.6, 0.196, 0.8);
}

struct MAP {
    double x, y;
} mp[1925][1085];

void init() {
    double x = -1.0, y = 1.0;
    for(int j = 1; j <= numH; j++) {
        x = -1.0;
        for(int i = 1; i <= numL; i++) {
            mp[i][j].x = x;
            mp[i][j].y = y;
            x += deltaL;
        }
        y -= deltaH;
    }
}

struct POINT {
    int num, x, y;
    int pxColor[1205][805];
    void clear(int lenX, int lenY) {
        for(int j = 1; j<= lenY; j++)
            for(int i = 1; i <= lenX; i++)
                pxColor[i][j] = 0;
        num = 0, x = y = -1;
    }
} P;

void display() {
    for(int j = 1; j <= numH; j++) {
        for(int i = 1; i <= numL; i++) {
            if(P.pxColor[i][j] != 0) {
                int t = P.pxColor[i][j];
                glBegin(GL_POLYGON);
                glColor3f(col[(t + 4) % 5 + 1].R, col[(t + 4) % 5 + 1].G, col[(t + 4) % 5 + 1].B);
                glVertex2f(mp[i][j].x, mp[i][j].y);
                glVertex2f(mp[i][j].x + deltaL, mp[i][j].y);
                glVertex2f(mp[i][j].x + deltaL, mp[i][j].y - deltaH);
                glVertex2f(mp[i][j].x, mp[i][j].y - deltaH);
                glEnd();
            }
            glBegin(GL_LINE_LOOP);
            glColor4f(col[0].R, col[0].G, col[0].B, pxSize * 1.0 / 100);
            glVertex2f(mp[i][j].x, mp[i][j].y);
            glVertex2f(mp[i][j].x + deltaL, mp[i][j].y);
            glVertex2f(mp[i][j].x + deltaL, mp[i][j].y - deltaH);
            glVertex2f(mp[i][j].x, mp[i][j].y - deltaH);
            glEnd();
        }
    }
}

void getLine(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int err = (dx > dy ? dx : -dy) / 2, e2;
    
    while(1) {
        P.pxColor[x0][y0] = (P.num + 1) / 2;
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy; x0 += sx;
        }
        if (e2 < dy) {
            err += dx; y0 += sy;
        }
    }
    cout << "OK\n";
}

void myMouse_callBack(GLFWwindow *window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double posX, posY;
        glfwGetCursorPos(window, &posX, &posY);
        int x = posX / pxSize + 1, y = posY / pxSize + 1;
        cout << x << ' ' << y << '\n';
        P.num++;
        if(P.num % 2) {
            P.x = x, P.y = y;
            P.pxColor[x][y] = (P.num + 1) / 2;
        }
        else {
            getLine(P.x, P.y, x, y);
            P.x = -1, P.y = -1;
        }
    }
}

void myKey_callBack(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_R && action == GLFW_PRESS) {
        P.clear(numL, numH);
    }
}

int main() {

    init();
    initColor();
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit())    return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(length, height, "Exp1", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, myMouse_callBack);
    glfwSetKeyCallback(window, myKey_callBack);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        display();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
