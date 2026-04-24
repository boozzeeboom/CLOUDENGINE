# CLOUDENGINE Unit Tests

Tests using Catch2 framework.

## Structure

```
tests/
├── CMakeLists.txt           # Test build configuration
├── ecs_components_test.cpp # ECS component tests
└── test_helpers.hpp        # Shared test utilities
```

## Running Tests

```bash
cd build
cmake --build . --config Debug
ctest --output-on-failure
```

## Test Naming

- `*_test.cpp` — test files
- Each TEST_CASE should be self-contained
- Use descriptive test names: `SECTION("Description")`

## Coverage Goals

| Component | Target |
|-----------|--------|
| ECS Components | 90%+ |
| ShipController | 80%+ |
| Network Sync | 70%+ |
