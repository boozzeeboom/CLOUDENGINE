f=open('log.txt','w')  
f.write('reading')  
f.close()  
with open('src/core/engine.cpp','r') as fp:  
    content=fp.read()  
f=open('log.txt','a')  
f.write(' done '+str(len(content)))  
f.close()  
