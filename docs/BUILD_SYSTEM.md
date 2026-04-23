# CLOUDENGINE Build System

## Compiler Configuration

**Visual Studio 2026 (18)** — ЕДИНСТВЕННЫЙ поддерживаемый компилятор.

| Component | Path |
|-----------|------|
| VS Root | `C:\Program Files\Microsoft Visual Studio\18\Community` |
| MSVC Toolset | `14.50.35717` |
| Windows SDK | `10.0.26100.0` |
| MSBuild | `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe` |

## CMake Generator

```
Visual Studio 18 2026
```

**ВАЖНО:** Никогда не использовать `Visual Studio 17 2022` — это для VS 2022, не совместим.

## Build Directory

**ЕДИНСТВЕННАЯ папка сборки:** `build/`

Старые папки сборки (для удаления):
- `build_test/` — удалить
- `build_test_new/` — удалить
- `build_test_ninja/` — удалить

## Build Script

```batch
build.bat
```

Автоматически:
1. Запускает VS Developer Command Prompt
2. Конфигурирует CMake с `Visual Studio 18 2026`
3. Собирает в `build/Debug/CloudEngine.exe`

## Manual Build (if needed)

```batch
REM Запустить из VS Developer Command Prompt:
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -host_arch=amd64 -arch=amd64

REM Configure
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Debug

REM Build
cmake --build build --config Debug
```

## Troubleshooting

### "Generator not found"
- Проверить что `vswhere` возвращает путь к VS 18
- Использовать `Visual Studio 18 2026` (НЕ 17 2022)

### "MSBuild not available"
- Вызвать `VsDevCmd.bat` перед cmake

### CMakeCache mismatch
- Удалить `build/` и `CMakeCache.txt` при смене генератора