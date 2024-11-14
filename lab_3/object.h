#include <glm/glm.hpp>
#include <vector>
#include <limits>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

class Object {
public:
    glm::vec3 color;        // 物体颜色
    float reflectivity;     // 物体的反射率

    Object(const glm::vec3& color, float reflectivity) 
        : color(color), reflectivity(reflectivity) {}

    // 纯虚函数，要求子类实现
    virtual bool intersect(const Ray& ray, float& t, glm::vec3& normal) const = 0;
};

class Sphere : public Object {
public:
    glm::vec3 center;
    float radius;

    Sphere(const glm::vec3& center, float radius, const glm::vec3& color, float reflectivity)
        : Object(color, reflectivity), center(center), radius(radius) {}

    // 重写 intersect 方法
    bool intersect(const Ray& ray, float& t, glm::vec3& normal) const override {
        glm::vec3 oc = ray.origin - center;
        float a = glm::dot(ray.direction, ray.direction);
        float b = 2.0f * glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) return false;

        t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
        if (t < 0) t = (-b + glm::sqrt(discriminant)) / (2.0f * a);
        if (t < 0) return false;

        glm::vec3 hitPoint = ray.origin + t * ray.direction;
        normal = glm::normalize(hitPoint - center);
        return true;
    }
};

class Wall : public Object {
public:
    glm::vec3 point;    // 墙的中心点
    glm::vec3 normal;   // 墙的法线方向
    glm::vec3 right;    // 墙的右方向向量（需要与 normal 垂直）
    float width;        // 墙的宽度
    float height;       // 墙的高度

    Wall(const glm::vec3& point, const glm::vec3& normal, const glm::vec3& right, float width, float height, const glm::vec3& color, float reflectivity)
        : Object(color, reflectivity), point(point), normal(glm::normalize(normal)), right(glm::normalize(right)), width(width), height(height) {
        // 确保 right 向量和 normal 向量是垂直的
        this->right = glm::normalize(right - glm::dot(right, normal) * normal);
    }

    // 重写 intersect 方法
    bool intersect(const Ray& ray, float& t, glm::vec3& outNormal) const override {
        float denom = glm::dot(normal, ray.direction);
        if (glm::abs(denom) > 1e-6) { // 检查光线是否平行于墙壁
            t = glm::dot(point - ray.origin, normal) / denom;
            if (t >= 0) {
                glm::vec3 hitPoint = ray.origin + t * ray.direction;

                // 计算墙的“上”方向向量
                glm::vec3 up = glm::cross(normal, right);

                // 将 hitPoint 转换为墙的局部坐标系
                glm::vec3 localHit = hitPoint - point;
                float hitX = glm::dot(localHit, right);  // 在 right 方向上的投影，代表水平位置
                float hitY = glm::dot(localHit, up);     // 在 up 方向上的投影，代表垂直位置

                // 检查是否在墙的宽度和高度范围内
                if (hitX >= -width / 2 && hitX <= width / 2 &&
                    hitY >= -height / 2 && hitY <= height / 2) {
                    outNormal = normal;
                    return true;
                }
            }
        }
        return false;
    }
};


struct Light {
    glm::vec3 position;
    glm::vec3 color;
};

struct Camera {
    glm::vec3 position;   // 相机位置
    glm::vec3 direction;  // 相机朝向方向
    float angle;          // 相机绕朝向旋转的角度
    float fov;            // 视野（FOV）
};