#include <iostream>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include <string>
#include <filesystem>
#include <curl/curl.h>

const std::string javaInstaller = "https://javadl.oracle.com/webapps/download/AutoDL?BundleId=236886_42970487e3af4f5aa5bca3f542482c60";
const std::string owner = "simonediberardino";
const std::string repo = "JBombLauncher";
const std::string fileName = "JBombLauncher.jar";

// Use _dupenv_s to safely retrieve environment variables
std::string getEnvVar(const char* varName) {
    char* value;
    size_t size;
    if (_dupenv_s(&value, &size, varName) == 0 && value != nullptr) {
        std::string result(value);
        free(value); // Don't forget to free the allocated memory
        return result;
    }
    return ""; // Return empty string if failed
}

// Function to download a file using libcurl
bool downloadFile(const std::string& url, const std::string& outputPath) {
    CURL* curl;
    FILE* fp;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        // Use fopen_s for safer file opening
        errno_t err = fopen_s(&fp, outputPath.c_str(), "wb");
        if (err != 0 || fp == nullptr) {
            std::cerr << "Failed to open file for writing: " << outputPath << std::endl;
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        res = curl_easy_perform(curl);
        fclose(fp);
        curl_easy_cleanup(curl);
        return res == CURLE_OK;
    }
    return false;
}

// Function to check if Java is installed
bool isJavaInstalled() {
    return system("where java >nul 2>&1") == 0;
}

// Function to execute a command
void executeCommand(const std::string& command) {
    system(command.c_str());
}

// Function to create a directory
bool createDirectory(const std::string& path) {
    return std::filesystem::create_directory(path);
}

// Function to create a symlink to the JBombLauncher.jar in a specified directory
void createSymlink(const std::string& target, const std::string& linkPath) {
    try {
        std::filesystem::create_symlink(target, linkPath);
        std::cout << "Symbolic link created: " << linkPath << std::endl;
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create symlink: " << e.what() << std::endl;
    }
}

// Function to get the Desktop path automatically
std::string getDesktopPath() {
    // Get the USERPROFILE environment variable
    std::string userProfile = getEnvVar("USERPROFILE");
    std::string defaultDesktopPath = userProfile + "\\Desktop"; // Default Desktop path

    // Check if the default Desktop path exists
    if (std::filesystem::exists(defaultDesktopPath)) {
        return defaultDesktopPath;
    }

    // Check for alternative Desktop paths (like on D: drive)
    std::string alternativeDesktopPath = "D:\\Desktop";
    if (std::filesystem::exists(alternativeDesktopPath)) {
        return alternativeDesktopPath;
    }

    // You can add more alternative paths if needed

    std::cerr << "Desktop path not found. Defaulting to user profile Desktop." << std::endl;
    return defaultDesktopPath;
}

// Main function
int main() {
    const std::string jbombDir = getEnvVar("ProgramFiles") + "\\JBomb";  // Dynamic Program Files directory
    const std::string jbombJar = jbombDir + "\\" + fileName;
    const std::string installer = jbombDir + "\\jdk8.exe"; // Save directly in the JBomb directory
    const std::string jbombJarUrl = "https://github.com/" + owner + "/" + repo + "/releases/latest/download/" + fileName;

    // Create JBomb directory if it does not exist
    if (!std::filesystem::exists(jbombDir)) {
        if (!createDirectory(jbombDir)) {
            std::cerr << "Failed to create JBomb directory." << std::endl;
            return 1;
        }
    }

    // Check for installed Java version
    if (isJavaInstalled()) {
        std::cout << "Java found, skipping installation." << std::endl;
    }
    else {
        std::cout << "Java not found, installing Java 8..." << std::endl;
        std::cout << "Downloading Java installer..." << std::endl;

        // Download Java installer directly to the JBomb directory
        if (downloadFile(javaInstaller, installer)) {
            std::cout << "Installing Java 8..." << std::endl;
            executeCommand(installer + " /s");
            std::filesystem::remove(installer);  // Remove installer after installation
        }
        else {
            std::cerr << "Failed to download Java installer. Exiting..." << std::endl;
            return 1;
        }
    }

    // Check for jbomblauncher.jar
    if (std::filesystem::exists(jbombJar)) {
        std::cout << "JBomb Launcher is already installed." << std::endl;
    }
    else {
        std::cout << "Downloading JBomb Launcher..." << std::endl;

        // Download JBomb launcher
        if (downloadFile(jbombJarUrl, jbombJar)) {
            std::cout << "JBomb Launcher downloaded successfully." << std::endl;
        }
        else {
            std::cerr << "Failed to download JBomb Launcher. Exiting..." << std::endl;
            return 1;
        }
    }

    // Launch the JBomb Launcher immediately after downloading
    std::cout << "Launching JBomb Launcher..." << std::endl;
    executeCommand("java -jar \"" + jbombJar + "\"");

    // Automatically determine the Desktop directory
    std::string desktopPath = getDesktopPath();

    std::string symlinkPath = desktopPath + "\\JBombLauncher.lnk";
    createSymlink(jbombJar, symlinkPath);

    std::cout << "Setup complete." << std::endl;
    return 0;
}
