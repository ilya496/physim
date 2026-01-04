#include "FileDialog.h"

#if defined(_WIN32)
// Windows implementation
#include <windows.h>
#include <shobjidl.h>

static std::wstring ToWide(const std::string& s)
{
    return std::wstring(s.begin(), s.end());
}

std::filesystem::path FileDialog::OpenFile(const std::string& title, const std::string& ext)
{
    IFileDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog)
    );

    if (FAILED(hr))
        return {};

    DWORD options;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);

    dialog->SetTitle(ToWide(title).c_str());

    COMDLG_FILTERSPEC filter{};
    std::wstring spec = L"*." + ToWide(ext);
    filter.pszName = spec.c_str();
    filter.pszSpec = spec.c_str();
    dialog->SetFileTypes(1, &filter);

    hr = dialog->Show(nullptr);
    if (FAILED(hr))
    {
        dialog->Release();
        return {};
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    if (FAILED(hr))
    {
        dialog->Release();
        return {};
    }

    PWSTR path = nullptr;
    item->GetDisplayName(SIGDN_FILESYSPATH, &path);

    std::filesystem::path result = path ? path : L"";

    CoTaskMemFree(path);
    item->Release();
    dialog->Release();

    return result;
}

std::filesystem::path FileDialog::SaveFile(const std::string& title, const std::string& ext)
{
    IFileSaveDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_FileSaveDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog)
    );

    if (FAILED(hr))
        return {};

    DWORD options;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);

    dialog->SetTitle(ToWide(title).c_str());
    dialog->SetDefaultExtension(ToWide(ext).c_str());

    COMDLG_FILTERSPEC filter{};
    std::wstring spec = L"*." + ToWide(ext);
    filter.pszName = spec.c_str();
    filter.pszSpec = spec.c_str();
    dialog->SetFileTypes(1, &filter);

    hr = dialog->Show(nullptr);
    if (FAILED(hr))
    {
        dialog->Release();
        return {};
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    if (FAILED(hr))
    {
        dialog->Release();
        return {};
    }

    PWSTR path = nullptr;
    item->GetDisplayName(SIGDN_FILESYSPATH, &path);

    std::filesystem::path result = path ? path : L"";

    CoTaskMemFree(path);
    item->Release();
    dialog->Release();

    return result;
}

std::filesystem::path FileDialog::SelectFolder(const std::string& title)
{
    IFileDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog)
    );

    if (FAILED(hr))
        return {};

    DWORD options;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dialog->SetTitle(ToWide(title).c_str());

    hr = dialog->Show(nullptr);
    if (FAILED(hr))
    {
        dialog->Release();
        return {};
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    if (FAILED(hr))
    {
        dialog->Release();
        return {};
    }

    PWSTR path = nullptr;
    item->GetDisplayName(SIGDN_FILESYSPATH, &path);

    std::filesystem::path result = path ? path : L"";

    CoTaskMemFree(path);
    item->Release();
    dialog->Release();

    return result;
}

#elif defined(__linux__)
// GTK / Zenity
// maybe some day in the future idk :/
#else
// macOS - not supported
// fuck Steve Jobs
#endif