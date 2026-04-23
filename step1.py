with open('src/core/engine.cpp','r') as f:  
    c=f.read()  
if 'void Engine::run() {'+chr(10)+'    CE_LOG_INFO(' in c:  
    open('log.txt','w').write('FOUND')  
else:  
    open('log.txt','w').write('NOT FOUND')  
