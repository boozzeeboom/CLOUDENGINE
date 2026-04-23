with open('src/core/engine.cpp', 'r', encoding='utf-8') as f:  
    content = f.read()  
  
# 1. Add START logs to run()  
old = 'void Engine::run() {' + chr(10) + '    CE_LOG_INFO(' + chr(34) + 'Engine running...' + chr(34) + ')'  
new = 'void Engine::run() {' + chr(10) + '    CE_LOG_INFO(' + chr(34) + 'Engine::run() - START' + chr(34) + ')' + chr(10) + '    CE_LOG_INFO(' + chr(34) + 'Engine::run() - starting main loop' + chr(34) + ')' + chr(10) + '    CE_LOG_INFO(' + chr(34) + 'Engine running...' + chr(34) + ')'  
content = content.replace(old, new)  
