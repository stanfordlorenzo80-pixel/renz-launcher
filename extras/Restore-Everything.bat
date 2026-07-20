@echo off
echo ============================================
echo   FORGE - Restore Everything I Disabled
echo ============================================
echo.
echo [1/3] Restoring boot apps (Roblox, Medal, Copilot, Edge, Epic, Opera)...
reg import "%USERPROFILE%\Desktop\Run-key-backup.reg" >nul 2>&1
echo     done.
echo.
echo [2/3] Restoring proxy gateways on boot (NyxProxy, OpenClaw)...
set "SU=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
if exist "%SU%\_disabled\NyxProxy.vbs" move /Y "%SU%\_disabled\NyxProxy.vbs" "%SU%\" >nul
if exist "%SU%\_disabled\start-proxy.vbs" move /Y "%SU%\_disabled\start-proxy.vbs" "%SU%\" >nul
echo     done.
echo.
echo [3/3] Restoring heavy MCP servers (ruflo, blender, claude-flow)...
copy /Y "%USERPROFILE%\.claude\_mcp-backup\.mcp.json" "%USERPROFILE%\.claude\.mcp.json" >nul 2>&1
copy /Y "%USERPROFILE%\.claude\_mcp-backup\mcpServers.json" "%USERPROFILE%\.claude\mcpServers.json" >nul 2>&1
echo     done.
echo.
echo ============================================
echo  ALL RESTORED. Reboot to bring back boot apps.
echo  Restart Claude Code to reload MCP servers.
echo ============================================
pause
