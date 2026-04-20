# flecs — ECS Library Reference

> **Версия**: v3.2.x (bundled single-header flecs.h + flecs.c)  
> **Лицензия**: MIT  
> **Источник**: https://github.com/SanderMertens/flecs  
> **Документация**: https://www.flecs.dev/flecs/

---

## Что такое flecs

flecs — высокопроизводительный ECS фреймворк для C/C++. В CLOUDENGINE используется C++ API (`flecs::world`).

**Ключевые возможности:**
- Archetype-based storage (SoA layout — cache-friendly)
- Многопоточные системы через Jobs
- Пайплайны с автоматическим управлением зависимостями
- Встроенные Relations (иерархия, пары)
- REST/HTTP inspector (отключаем в production)
- Observers (реакция на добавление/удаление компонентов)

---

## Инициализация

```cpp
#include "flecs.h"

int main() {
    flecs::world world;           // создать мир
    world.set_target_fps(60.0f); // опционально
    
    // ... регистрация компонентов и систем ...
    
    while (world.progress()) {   // основной цикл
        // world.progress(delta_time) для фиксированного шага
    }
}
```

---

## Компоненты (Components)

Компоненты — **только данные**, никакой логики!

```cpp
// Определение компонентов
struct Position {
    float x, y, z;
};

struct Velocity {
    float x, y, z;
};

struct Health {
    float current;
    float max;
};

struct RenderMesh {
    uint32_t vao;
    uint32_t indexCount;
};

// Тэг-компонент (без данных)
struct IsPlayer {};
struct IsVisible {};
```

---

## Сущности (Entities)

```cpp
flecs::world world;

// Создание сущности
flecs::entity player = world.entity("Player");

// Добавление компонентов
player.add<IsPlayer>()
      .set<Position>({0.0f, 100.0f, 0.0f})
      .set<Velocity>({0.0f, 0.0f, 0.0f})
      .set<Health>({100.0f, 100.0f});

// Чтение компонента
const Position* pos = player.get<Position>();
if (pos) {
    // используем pos->x, pos->y, pos->z
}

// Изменение компонента
player.set<Position>({1.0f, 100.0f, 0.0f});

// Через мутацию (ref)
player.get_mut<Health>([](Health& h) {
    h.current -= 10.0f;
});

// Удаление компонента
player.remove<Velocity>();

// Удаление сущности
player.destruct();

// Проверка
bool alive = player.is_alive();
bool hasVelocity = player.has<Velocity>();
```

---

## Системы (Systems)

Системы — вся логика. Итерируют сущности с нужными компонентами.

```cpp
// Простая система — лямбда
world.system<Position, const Velocity>("MovementSystem")
    .each([](Position& pos, const Velocity& vel) {
        pos.x += vel.x;
        pos.y += vel.y;
        pos.z += vel.z;
    });

// Система с delta_time
world.system<Position, const Velocity>("MovementSystem")
    .each([](flecs::iter& it, size_t i, Position& pos, const Velocity& vel) {
        float dt = it.delta_time();
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        pos.z += vel.z * dt;
    });

// Система как класс (рекомендуется для CLOUDENGINE)
struct MovementSystem {
    MovementSystem(flecs::world& world) {
        world.system<Position, const Velocity>("MovementSystem")
            .kind(flecs::OnUpdate)
            .each([](flecs::iter& it, size_t i, Position& p, const Velocity& v) {
                float dt = it.delta_time();
                p.x += v.x * dt;
                p.y += v.y * dt;
                p.z += v.z * dt;
            });
    }
};

// Регистрация модуля
world.import<MovementSystem>();
```

---

## Пайплайны (Pipelines)

```cpp
// Встроенные фазы (порядок выполнения):
// flecs::OnLoad          → загрузка данных
// flecs::PostLoad        → после загрузки
// flecs::PreUpdate       → до обновления
// flecs::OnUpdate        → основное обновление (дефолтная фаза)
// flecs::OnValidate      → валидация
// flecs::PostUpdate      → после обновления
// flecs::PreStore        → до сохранения
// flecs::OnStore         → рендеринг / запись

// Кастомный пайплайн для CLOUDENGINE
flecs::entity Physics = world.entity("PhysicsPhase").add(flecs::Phase);
flecs::entity Render  = world.entity("RenderPhase").add(flecs::Phase)
    .depends_on(Physics);

// Система в кастомной фазе
world.system<Position, const Velocity>("PhysicsUpdate")
    .kind(Physics)
    .each([](flecs::iter& it, size_t i, Position& p, const Velocity& v) {
        p.x += v.x * it.delta_time();
    });

world.system<const Position, const RenderMesh>("RenderMeshes")
    .kind(Render)
    .each([](const Position& p, const RenderMesh& mesh) {
        // отправить draw call
    });
```

---

## Запросы (Queries)

```cpp
// Разовый запрос
world.each([](flecs::entity e, Position& p, const Velocity& v) {
    p.x += v.x;
});

// Кэшированный запрос (создать один раз, итерировать многократно)
auto q = world.query<Position, const Velocity>();

// В системе / цикле:
q.each([](Position& p, const Velocity& v) {
    p.x += v.x;
});

// Запрос с фильтром
auto q2 = world.query_builder<Position>()
    .with<IsPlayer>()
    .without<IsVisible>()
    .build();

// Итерация через iter (для массового доступа — быстрее)
q.iter([](flecs::iter& it, Position* p, const Velocity* v) {
    for (auto i : it) {
        p[i].x += v[i].x * it.delta_time();
    }
});
```

---

## Observers (Наблюдатели)

```cpp
// Реакция на добавление компонента
world.observer<Position>("OnPositionAdded")
    .event(flecs::OnAdd)
    .each([](flecs::iter& it, size_t i, Position& p) {
        ecs_trace("Position added to entity %s",
            it.entity(i).name().c_str());
    });

// Реакция на удаление
world.observer<Health>("OnHealthRemoved")
    .event(flecs::OnRemove)
    .each([](Health& h) {
        // cleanup
    });
```

---

## Иерархия (Parent-Child)

```cpp
// Создание иерархии
flecs::entity ship = world.entity("Airship");
flecs::entity engine = world.entity("Engine").child_of(ship);
flecs::entity cannon = world.entity("Cannon").child_of(ship);

// Итерация детей
ship.children([](flecs::entity child) {
    ecs_trace("Child: %s", child.name().c_str());
});

// Получение родителя
flecs::entity parent = engine.parent();
```

---

## Синглтоны (Singleton / World Components)

```cpp
// Глобальные данные (конфиг, время, ресурсы)
struct TimeData {
    float total;
    float delta;
    uint64_t frame;
};

struct EngineConfig {
    int screenWidth;
    int screenHeight;
    bool vsync;
};

// Установка синглтона
world.set<TimeData>({0.0f, 0.0f, 0});
world.set<EngineConfig>({1280, 720, true});

// Чтение
const TimeData* time = world.get<TimeData>();
world.get_mut<TimeData>([](TimeData& t) {
    t.frame++;
    t.total += t.delta;
});

// В системе
world.system("UpdateTime")
    .kind(flecs::PreUpdate)
    .run([](flecs::iter& it) {
        auto* t = it.world().get_mut<TimeData>();
        t->delta = it.delta_time();
        t->total += t->delta;
        t->frame++;
    });
```

---

## Модули (Modules)

Рекомендуемый паттерн для CLOUDENGINE — каждая подсистема как модуль:

```cpp
struct RenderingModule {
    RenderingModule(flecs::world& world) {
        // Регистрация компонентов
        world.component<RenderMesh>();
        world.component<Transform>();
        
        // Регистрация систем
        world.system<const Transform, const RenderMesh>("DrawSystem")
            .kind(flecs::OnStore)
            .iter([](flecs::iter& it, 
                     const Transform* transforms,
                     const RenderMesh* meshes) {
                for (auto i : it) {
                    // draw call
                }
            });
    }
};

// В main:
world.import<RenderingModule>();
```

---

## Производительность

```cpp
// ПРАВИЛО: Системы с iter() быстрее each() для больших наборов
// each() — удобно для прототипов
// iter() — production код

// ПРАВИЛО: Запросы создавать один раз (member переменная системы)
struct PhysicsSystem {
    flecs::query<Position, const Velocity> _query;
    
    PhysicsSystem(flecs::world& w) 
        : _query(w.query<Position, const Velocity>()) {
        // система через run() вместо each() для zero-alloc
        w.system("PhysicsRun")
            .run([this](flecs::iter& it) {
                _query.iter([](flecs::iter& qit, 
                               Position* p, 
                               const Velocity* v) {
                    for (auto i : qit) {
                        p[i].x += v[i].x * qit.delta_time();
                        p[i].y += v[i].y * qit.delta_time();
                    }
                });
            });
    }
};
```

---

## Многопоточность

```cpp
// Включение многопоточности
world.set_threads(4);

// Система автоматически параллелится если .multi_threaded()
world.system<Position, const Velocity>("ParallelPhysics")
    .multi_threaded()
    .each([](Position& p, const Velocity& v) {
        p.x += v.x;
    });
```

---

## Ссылки

- [Official Quickstart](https://www.flecs.dev/flecs/md_docs_2Quickstart.html)
- [C++ API Reference](https://www.flecs.dev/flecs/group__cpp.html)
- [GitHub](https://github.com/SanderMertens/flecs)
- [Flecs Explorer](https://www.flecs.dev/explorer/) — визуальный инспектор ECS
