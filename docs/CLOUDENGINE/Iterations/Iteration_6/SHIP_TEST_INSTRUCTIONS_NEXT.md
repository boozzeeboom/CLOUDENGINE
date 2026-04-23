# CLOUDENGINE — Ship Physics Test Instructions
## Дата: 2026-04-23
## Статус: Build с debug логированием готов

---

## Подготовка

### Уже сделано:
1. Добавлено debug логирование в `ShipControllerSystem`
2. Создан `test_ship_physics.bat` для запуска с чистыми логами
3. Создан `analyze_ship_test.bat` для анализа результатов

---

## Как тестировать

### Шаг 1: Запустить тестовый скрипт
```
cd c:\CLOUDPROJECT\CLOUDENGINE
test_ship_physics.bat
```

### Шаг 2: Тестировать управление
1. При запуске нажми **правую кнопку мыши (RMB)** — захватит курсор
2. **W** — движение вперёд (через Jolt applyForce)
3. **A/D** — поворот влево/вправо (через Jolt applyTorque)
4. **Q/E** — движение вверх/вниз
5. **Shift** — буст (2x тяга)
6. **ESC** — выход

### Шаг 3: Проверить поведение
- **Gravity**: корабль должен падать вниз когда не двигаешься
- **Thrust**: при нажатии W корабль двигается
- **Rotation**: A/D поворачивают корабль

### Шаг 4: Анализ логов
После закрытия приложения:
```
analyze_ship_test.bat
```

---

## Что логируется (в logs/cloudengine.log)

### Ожидаемые логи (успех):
```
[INFO] Ship components registered
[INFO] Ship systems registered
[INFO] ShipInput: CURSOR CAPTURED (RMB)
[TRACE] ShipController: entity=LocalPlayer, bodyId=X, fwd:0.0/vert:0.0/yaw:0.0/pitch:0.0/boost:no
[TRACE] ShipController: entity=LocalPlayer, bodyId=X, fwd:1.0/vert:0.0/yaw:0.0/pitch:0.0/boost:no  <-- W нажата
[DEBUG] ShipController: fwd force=(0.0,0.0,50000.0) bodyId=X  <-- сила применена
```

### Проблемы и их признаки:

| Проблема | Признак в логах |
|----------|-----------------|
| Система не работает | Нет "ShipController" в логах |
| Jolt body не создан | "has invalid JoltBodyId" |
| Курсор не захвачен | Нет "CURSOR CAPTURED" |
| Сила не применяется | Нет "applying fwd force" |
| Краш | Логи обрываются |

---

## Файлы результатов

После анализа будут созданы:
- `logs/test_analysis.txt` — итоговый анализ
- `logs/cloudengine_TIMESTAMP.log` — backup логов

---

## Заполни результаты:

### Тест 1: Базовая функциональность
- [ ] Окно открывается?
- [ ] Курсор захватывается при RMB?
- [ ] "CURSOR CAPTURED" в логах?
- [ ] "ShipController" в логах?

### Тест 2: Управление
- [ ] W двигает корабль?
- [ ] A/D поворачивает корабль?
- [ ] Q/E двигает вверх/вниз?
- [ ] Shift работает как буст?

### Тест 3: Gravity
- [ ] Корабль падает когда idle?
- [ ] "applying fwd force" появляется при нажатии W?

### Итог:
```
РАБОТАЕТ / ЧАСТИЧНО РАБОТАЕТ / НЕ РАБОТАЕТ
```

### Проблемы (если есть):
1. ...
2. ...
3. ...

---

## Следующий шаг

Сообщи результаты — скопируй содержимое `logs\test_analysis.txt` или опиши что увидел.
