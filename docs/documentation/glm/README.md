# GLM 1.0.x — Math Library Reference для CLOUDENGINE

> **Версия**: 1.0.x (master)  
> **Лицензия**: MIT  
> **Источник**: https://github.com/g-truc/glm  
> **Документация**: https://glm.g-truc.net/

---

## Подключение

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // translate, rotate, scale, perspective
#include <glm/gtc/type_ptr.hpp>          // value_ptr (для передачи в OpenGL)
#include <glm/gtx/quaternion.hpp>        // кватернионы (опционально)
```

---

## Базовые типы

```cpp
// Векторы
glm::vec2  pos2d{1.0f, 2.0f};
glm::vec3  pos3d{1.0f, 2.0f, 3.0f};
glm::vec4  color{1.0f, 0.0f, 0.0f, 1.0f};  // RGBA

// Матрицы (column-major, как в OpenGL!)
glm::mat4 identity = glm::mat4(1.0f);  // единичная
glm::mat3 rotation3x3;

// Целочисленные
glm::ivec2 screenPos{800, 600};
glm::uvec2 texSize{1024, 1024};

// Доступ к компонентам
glm::vec3 v{1.0f, 2.0f, 3.0f};
float x = v.x;  // или v.r, v.s
float y = v.y;  // или v.g, v.t
float z = v.z;  // или v.b, v.p

// Swizzle
glm::vec2 xy = v.xy;     // не поддерживается в GLM по умолчанию
// Используй: glm::vec2(v.x, v.y)
```

---

## Трансформации

```cpp
// Создание матрицы трансформации (T * R * S порядок):
glm::mat4 model = glm::mat4(1.0f);

// Перемещение
model = glm::translate(model, glm::vec3(1.0f, 0.0f, 0.0f));

// Поворот (угол в радианах, ось)
model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

// Масштаб
model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));

// Альтернатива — собрать Transform компонент:
struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};  // euler angles в градусах
    glm::vec3 scale{1.0f};
    
    glm::mat4 GetMatrix() const {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.x), {1,0,0});
        m = glm::rotate(m, glm::radians(rotation.y), {0,1,0});
        m = glm::rotate(m, glm::radians(rotation.z), {0,0,1});
        m = glm::scale(m, scale);
        return m;
    }
};
```

---

## Проекционные матрицы

```cpp
// Перспективная проекция (для 3D)
float fovY   = glm::radians(60.0f);  // вертикальный FOV
float aspect = (float)width / height;
float near   = 0.1f;
float far    = 100000.0f;  // для большого мира!

glm::mat4 projection = glm::perspective(fovY, aspect, near, far);

// Ортографическая проекция (для UI/2D)
glm::mat4 ortho = glm::ortho(
    0.0f, (float)width,   // left, right
    (float)height, 0.0f,  // bottom, top (y вниз)
    -1.0f, 1.0f           // near, far
);

// Бесконечная дальность (для огромного мира)
// Используй reversed-Z для лучшей точности:
glm::mat4 projInfinite = glm::infinitePerspective(fovY, aspect, near);
```

---

## Камера (View Matrix)

```cpp
// lookAt — стандартная камера
glm::vec3 eye    = {0.0f, 10.0f, 50.0f};  // позиция камеры
glm::vec3 center = {0.0f, 0.0f, 0.0f};    // точка взгляда
glm::vec3 up     = {0.0f, 1.0f, 0.0f};    // вверх

glm::mat4 view = glm::lookAt(eye, center, up);

// FPS камера через Euler angles:
struct Camera {
    glm::vec3 position{0.0f, 5.0f, 10.0f};
    float yaw   = -90.0f;  // горизонтальный угол
    float pitch = 0.0f;    // вертикальный угол
    float fov   = 60.0f;
    
    glm::vec3 GetForward() const {
        return glm::normalize(glm::vec3{
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        });
    }
    
    glm::mat4 GetView() const {
        return glm::lookAt(position, position + GetForward(), {0,1,0});
    }
    
    glm::mat4 GetProjection(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, 0.1f, 500000.0f);
    }
};
```

---

## Математические функции

```cpp
// Длина вектора
float len = glm::length(v);

// Нормализация
glm::vec3 norm = glm::normalize(v);

// Скалярное произведение
float dot = glm::dot(a, b);

// Векторное произведение
glm::vec3 cross = glm::cross(a, b);

// Расстояние
float dist = glm::distance(a, b);

// Линейная интерполяция
glm::vec3 lerped = glm::mix(a, b, 0.5f);  // 50% между a и b

// Clamp
float clamped = glm::clamp(value, 0.0f, 1.0f);

// Min/Max
glm::vec3 minV = glm::min(a, b);
glm::vec3 maxV = glm::max(a, b);

// Степень, корень
float powered = glm::pow(base, exp);
float rooted  = glm::sqrt(value);

// Конвертация угол
float rad = glm::radians(180.0f);  // → π
float deg = glm::degrees(glm::pi<float>());  // → 180
```

---

## Передача матриц в OpenGL

```cpp
// Через glm::value_ptr (возвращает float*)
glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));

// Для vec3, vec4:
glUniform3fv(location, 1, glm::value_ptr(vec));
glUniform4fv(location, 1, glm::value_ptr(vec));

// Пример полной передачи MVP:
glm::mat4 model      = transform.GetMatrix();
glm::mat4 view       = camera.GetView();
glm::mat4 projection = camera.GetProjection(aspect);

glUniformMatrix4fv(uModel,      1, GL_FALSE, glm::value_ptr(model));
glUniformMatrix4fv(uView,       1, GL_FALSE, glm::value_ptr(view));
glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));
```

---

## Кватернионы

```cpp
#include <glm/gtx/quaternion.hpp>

// Создание кватерниона
glm::quat q = glm::identity<glm::quat>();
glm::quat fromEuler = glm::quat(glm::vec3(
    glm::radians(pitch), 
    glm::radians(yaw), 
    glm::radians(roll)
));

// Кватернион из оси и угла
glm::quat axisAngle = glm::angleAxis(glm::radians(45.0f), glm::vec3(0,1,0));

// Поворот вектора
glm::vec3 rotated = q * glm::vec3(0, 0, -1);

// В матрицу
glm::mat4 rotMat = glm::toMat4(q);

// Интерполяция (Slerp)
glm::quat slerped = glm::slerp(q1, q2, t);

// Нормализация
q = glm::normalize(q);
```

---

## Большой мир (Floating Origin)

```cpp
// Для мира 350,000 units — используй double для мировых координат
// Конвертируй в float только для рендеринга относительно камеры

struct WorldPosition {
    double x, y, z;  // абсолютные координаты мира
};

struct FloatingOrigin {
    glm::dvec3 origin{0.0};  // текущий origin мира
};

// Рендер-позиция = мировая - origin (fits in float)
glm::vec3 RenderPos(const WorldPosition& wp, const FloatingOrigin& fo) {
    return glm::vec3{
        wp.x - fo.origin.x,
        wp.y - fo.origin.y,
        wp.z - fo.origin.z
    };
}
```

---

## Ссылки

- [GLM Manual](https://glm.g-truc.net/0.9.9/api/a00178.html)
- [GitHub](https://github.com/g-truc/glm)
- [GLM Cheat Sheet](https://github.com/g-truc/glm/blob/master/manual.md)
