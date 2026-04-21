# Cline Configuration for Synapse

**Prerequisites:**
1. Install Cline extension from VS Code marketplace
2. Synapse server running on http://localhost:8000

---

## Method 1: Add to VS Code settings.json

Press `Ctrl+Shift+P` → "Open User Settings (JSON)" → Add:

```json
{
  // Cline API Configuration
  "cline.customApiProviders": {
    "synapse": {
      "baseURL": "http://localhost:8000/v1",
      "apiKey": "cloudengine-memory-key",
      "model": "synapse",
      "organization": null
    }
  },
  "cline.customProvider": "synapse",
  
  // Or use OpenAI-compatible endpoint directly:
  "cline.apiProvider": "openai",
  "cline.apiUrl": "http://localhost:8000/v1",
  "cline.apiKey": "cloudengine-memory-key",
  "cline.model": "synapse"
}
```

---

## Method 2: Via Cline Settings UI

1. Open Cline extension (click Cline icon in sidebar)
2. Click gear icon for settings
3. Set:
   - **API Provider:** OpenAI Compatible
   - **Base URL:** `http://localhost:8000/v1`
   - **API Key:** `cloudengine-memory-key`
   - **Model ID:** `synapse`

---

## Verify Connection

After configuration, Cline should:
1. Show "Synapse" as the active provider
2. Be able to search your indexed documents

Test with a query about CLOUDENGINE architecture.

---

## Restart Required

Changes to settings.json require restarting VS Code or reloading window (`Ctrl+Shift+P` → "Reload Window")