@echo off
copy /Y "%USERPROFILE%\.claude\_mcp-backup\.mcp.json" "%USERPROFILE%\.claude\.mcp.json" >nul
copy /Y "%USERPROFILE%\.claude\_mcp-backup\mcpServers.json" "%USERPROFILE%\.claude\mcpServers.json" >nul
echo Heavy MCP servers (ruflo, blender, claude-flow) RE-ENABLED. Restart Claude Code to load them.
pause
