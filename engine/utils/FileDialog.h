#pragma once

#include <string>
#include <filesystem>

class FileDialog
{
public:
    static std::filesystem::path OpenFile(const std::string& title, const std::string& ext);
    static std::filesystem::path SaveFile(const std::string& title, const std::string& ext);
    static std::filesystem::path SelectFolder(const std::string& title);
};