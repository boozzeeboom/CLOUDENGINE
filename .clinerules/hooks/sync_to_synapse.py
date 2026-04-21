"""
Sync cached session data to Synapse Memory
Called by session-stop.ps1 hook
"""
import sys
import json
import os

def sync_to_synapse():
    cache_file = os.path.join(os.path.dirname(__file__), '..', '..', '.cline', 'synapse_session_cache.json')
    
    if not os.path.exists(cache_file):
        print("No cache file found")
        return
    
    with open(cache_file, 'r') as f:
        raw = json.load(f)
    
    # Handle both dict and list formats
    if isinstance(raw, dict):
        sessions = [raw]
    else:
        sessions = raw
    
    # Add synapse-memory to path - use absolute path to Python
    sys.path.insert(0, 'C:/CLOUDPROJECT/synapse-memory')
    
    from app.services.memory import MemoryService
    memory = MemoryService()
    
    for session in sessions:
        session_id = session.get('session_id', 'default')
        context = session.get('context', {})
        timestamp = session.get('timestamp', '')
        
        content = json.dumps({
            'timestamp': timestamp,
            'session_num': session.get('session_num', 1),
            **context
        })
        
        memory.store_session_memory(
            session_id=session_id,
            content=content,
            memory_type='task_context'
        )
    
    # Clear cache after sync
    os.remove(cache_file)
    print("Synced {} sessions to Synapse Memory".format(len(sessions)))

if __name__ == "__main__":
    try:
        sync_to_synapse()
        print("SUCCESS")
    except Exception as e:
        print(f"ERROR: {e}")
        sys.exit(1)