with open('src/core/engine.cpp','r') as f:  
    c=f.read()  
old='void Engine::run() {'+chr(10)+'    CE_LOG_INFO('  
new='void Engine::run() {'+chr(10)+'    CE_LOG_INFO(\"Engine::run() - START\");'+chr(10)+'    CE_LOG_INFO(\"Engine::run() - starting main loop\");'+chr(10)+'    CE_LOG_INFO('  
c=c.replace(old,new,1)  
with open('src/core/engine.cpp','w') as f:  
    f.write(c)  
