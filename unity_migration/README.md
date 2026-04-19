# Unity Migration Files - Project C: The Clouds

Данный каталог содержит файлы из Unity6 проекта, необходимые для миграции на движок CLOUDENGINE.

## Структура каталога

### Scripts/
Основные C# скрипты проекта:
- `Core/` - Ядро движка (WorldGenerator, FloatingOrigin, NetworkManagerController и т.д.)
- `Player/` - Управление игроком (PlayerController, ShipController, NetworkPlayer)
- `Ship/` - Система кораблей (ShipModule, MeziyModule, WindZone)
- `UI/` - Пользовательский интерфейс
- `World/` - Генерация мира (MountainMeshGenerator, ChunkLoader, Streaming)
- `Trade/` - Торговая система

### Data/
Конфигурационные данные:
- `AltitudeCorridors/` - Конфигурации высотных коридоров
- `Clouds/` - Конфигурации облаков
- `Modules/` - Данные модулей кораблей
- `WindZones/` - Конфигурации ветровых зон
- `World/` - Данные мира (Massifs, BiomeProfiles)

### Materials/
Материалы:
- `Clouds/` - Облачные материалы (Lower, Middle, Upper, Veil)
- `World/` - Материалы мира

### Prefabs/
Префабы объектов

### Settings/
Настройки URP рендерера

### Shaders_Art/, Shaders_Root/
Шейдеры:
- `CloudGhibli.shader` - Облачный шейдер в стиле Ghibli
- `VeilShader.shader` - Шейдер вуали/пелены

### Scenes/
Сцены Unity

### Packages/
- `manifest.json` - Манифест Unity пакетов
- `packages-lock.json` - Блокировка версий пакетов

## Документация

### docs_unity6/
Документы по Unity 6 API и архитектуре:
- `Unity6_API_Reference.md` - API Reference
- `Unity6_Architecture.md` - Архитектура Unity 6
- `Unity6_BestPractices.md` - Best Practices
- `UNITY6_URP_SETUP.md` - Настройка URP

### docs_gdd/
Game Design Documents:
- `GDD_00_Overview.md` - Обзор
- `GDD_01_Core_Gameplay.md` - Геймплей
- `GDD_10_Ship_System.md` - Система кораблей
- `GDD_11_Inventory_Items.md` - Инвентарь
- `GDD_12_Network_Multiplayer.md` - Сеть
- и др.

### docs_ships/
Документы по системе кораблей:
- `SHIP_CLASS_PRESETS.md` - Пресеты классов
- `ShipRegistry.md` - Реестр кораблей
- `HOWTO_CREATE_SHIP.md` - Как создать корабль

### docs_world/
Документы по миру и генерации:
- `MOUNTAIN_MESH_V2_PLAN.md` - Генерация гор
- `LargeScaleMMO/` - Масштабные MMO решения
- `LargeScaleMMO/FLOAT_PRECISION_ISSUE.md` - Проблема точности координат
- `LargeScaleMMO/ADR-0002_WorldStreaming_Architecture.md` - Архитектура стриминга

### Корневые .md файлы
- `05_ENGINE_CORE_INTEGRATION.md` - Интеграция движка
- `LARGE_WORLD_SOLUTIONS.md` - Масштабные решения
- `NGO_BEST_PRACTICES.md` - Best Practices NGO
- `NETWORK_ARCHITECTURE.md` - Архитектура сети
- `INVENTORY_SYSTEM.md` - Система инвентаря
- `TRADE_SYSTEM_RAG.md` - Торговая система
- `SHIP_SYSTEM_DOCUMENTATION.md` - Документация кораблей

## Ключевые компоненты для миграции

### Сетевая подсистема (NGO)
- NetworkManagerController
- NetworkPlayer
- NetworkPlayerSpawner
- ChunkNetworkSpawner
- FloatingOriginMP

### Генерация мира
- WorldGenerator
- MountainMeshGenerator / MountainMeshBuilderV2
- WorldStreamingManager
- ChunkLoader
- ProceduralNoiseGenerator

### Система кораблей
- ShipController
- ShipModuleManager
- MeziyModuleActivator
- WindZone / WindZoneData
- TurbulenceEffect
- AltitudeCorridorSystem

### UI система
- UIManager
- TradeUI
- InventoryUI
- NetworkUI

### Floating Origin (большие миры)
- FloatingOrigin.cs - Смещение мира относительно камеры
- FloatingOriginMP.cs - MP версия

## Заметки по миграции

1. NGO (Netcode for GameObjects) - требует замены на сетевую подсистему CLOUDENGINE
2. Unity Physics - адаптировать к физике движка
3. URP шейдеры - конвертировать в формат движка
4. Input System - использовать InputReader паттерн
5. Streaming - адаптировать ChunkLoader/WorldStreamingManager

## .gitignore

В каталоге есть `.gitignore` для исключения из репозитория:

### Исключённые типы файлов
- `*.meta` - Unity метафайлы (автогенерируемые)
- `*.asset` - Unity бинарные ассеты
- `*.mat` - материалы (бинарные)
- `*.prefab` / `*.prefab.meta` - префабы (YAML complexity)
- `*.unity` / `*.unity.meta` - сцены
- `*.dll`, `*.pdb` - бинарные библиотеки
- `*.shader` - шейдеры (текстовые, но можно исключить)

### Исключённые каталоги
- `TextMesh Pro/` - автогенерируемые шрифты
- `Temp/`, `temp/` - временные Unity файлы
- `Library/` - кэш Unity
- `Logs/` - логи
- `Build/`, `build/`, `Builds/` - артефакты сборки
- `.vscode/`, `.idea/` - IDE файлы
- `*Settings.asset` - URP/HDRP настройки

### Оставленные файлы (для миграции)
- Все `.cs` скрипты
- `.json` конфигурации (manifest.json, InputAction)
- `.md` документация
- `.hlsl`, `.cginc` шейдерные инклюды
