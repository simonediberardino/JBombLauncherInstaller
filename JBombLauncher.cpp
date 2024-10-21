#include <iostream>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include <string>
#include <filesystem>
#include <winhttp.h> // Include the WinHTTP library

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

bool downloadFile(const std::string& url, const std::string& outputPath) {
    std::string host;
    std::string path;

    // Extract host and path from the URL
    size_t protocolEnd = url.find("://") + 3; // Find end of protocol
    size_t pathStart = url.find('/', protocolEnd); // Find start of path
    host = url.substr(protocolEnd, pathStart - protocolEnd); // Extract host
    path = url.substr(pathStart); // Extract path

    // Initialize WinHTTP session
    HINTERNET hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        std::cerr << "WinHttpOpen failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Connect using HTTPS port
    HINTERNET hConnect = WinHttpConnect(hSession, std::wstring(host.begin(), host.end()).c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        std::cerr << "WinHttpConnect failed. Error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Open a request with INTERNET_FLAG_SECURE for HTTPS
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", std::wstring(path.begin(), path.end()).c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        std::cerr << "WinHttpOpenRequest failed. Error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Send the request
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
        std::cerr << "WinHttpSendRequest failed. Error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Receive the response
    if (WinHttpReceiveResponse(hRequest, NULL) == FALSE) {
        std::cerr << "WinHttpReceiveResponse failed. Error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Open file for writing
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Failed to open output file: " << outputPath << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Read data from the response and write to file
    DWORD bytesRead;
    char buffer[4096];
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outputFile.write(buffer, bytesRead);
    }

    // Cleanup
    outputFile.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}

// Function to check if Java is installed
bool isJavaInstalled() {
    return system("where java >nul 2>&1") == 0;
}

// Function to create a directory
bool createDirectory(const std::string& path) {
    return std::filesystem::create_directory(path);
}

// Function to launch the JBomb Launcher in a new console
void launchGame(const std::string& command) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring commandW(command.begin(), command.end()); // Convert to wide string

    // Create a new console for the process
    if (!CreateProcessW(nullptr, &commandW[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        std::cerr << "Failed to launch the game. Error: " << GetLastError() << std::endl;
        return; // Exit the function if process creation fails
    }

    // Close handles to the process and thread, but let the process run independently
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// Function to display welcome messages in a box
void displayWelcomeMessages() {
    std::string welcomeMessage = "Welcome to JBomb Launcher!";
    std::string checkRequirements = "We'll check your system requirements and launch your launcher shortly.";
    std::string patienceMessage = "Please be patient as we get everything ready for you!";

    // Manually calculate the maximum length
    size_t maxLength = welcomeMessage.length(); // Start with the first message length

    // Compare lengths of the other messages
    if (checkRequirements.length() > maxLength) {
        maxLength = checkRequirements.length();
    }
    if (patienceMessage.length() > maxLength) {
        maxLength = patienceMessage.length();
    }

    // Create the top border
    std::string border(maxLength + 4, '*');
    std::cout << border << std::endl;

    // Print each message with padding
    std::cout << "* " << welcomeMessage << std::string(maxLength - welcomeMessage.length(), ' ') << " *" << std::endl;
    std::cout << "* " << checkRequirements << std::string(maxLength - checkRequirements.length(), ' ') << " *" << std::endl;
    std::cout << "* " << patienceMessage << std::string(maxLength - patienceMessage.length(), ' ') << " *" << std::endl;
    std::cout << border << std::endl << std::endl;
}

// Function to setup directories
void setupDirectories(const std::string& localAppDataDir, const std::string& binDir) {
    // Create JBomb directory if it does not exist
    if (!std::filesystem::exists(localAppDataDir)) {
        createDirectory(localAppDataDir);
    }

    // Create the bin directory if it does not exist
    if (!std::filesystem::exists(binDir)) {
        createDirectory(binDir);
    }
}

// Function to handle Java installation
bool handleJavaInstallation(const std::string& installer) {
    if (!isJavaInstalled()) {
        std::cout << "Java not found, downloading and installing Java..." << std::endl;
        if (downloadFile(javaInstaller, installer)) {
            system((installer + " /s").c_str());
            std::cout << "Java installed successfully." << std::endl;
            std::filesystem::remove(installer); // Remove installer after installation
            return true;
        }
        else {
            std::cerr << "Failed to download Java installer." << std::endl;
            return false;
        }
    }
    return true;
}

// Function to check and download JBomb Launcher
bool checkAndDownloadJBombLauncher(const std::string& jbombJar, const std::string& jbombJarUrl) {
    if (!std::filesystem::exists(jbombJar)) {
        std::cout << "Downloading JBomb Launcher..." << std::endl;
        if (downloadFile(jbombJarUrl, jbombJar)) {
            std::cout << "JBomb Launcher downloaded successfully." << std::endl;
            return true;
        }
        else {
            std::cerr << "Failed to download JBomb Launcher." << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    const std::string localAppDataDir = getEnvVar("LOCALAPPDATA") + "\\JBomb";  // Store files in Local AppData
    const std::string binDir = localAppDataDir + "\\bin";  // Create a bin directory under JBomb
    const std::string jbombJar = binDir + "\\" + fileName;  // Path for JBombLauncher.jar
    const std::string installer = localAppDataDir + "\\jdk8.exe";  // Java installer path in Local AppData
    const std::string jbombJarUrl = "https://github.com/" + owner + "/" + repo + "/releases/latest/download/" + fileName;

    setupDirectories(localAppDataDir, binDir);
    displayWelcomeMessages();

    // Handle Java installation
    if (!handleJavaInstallation(installer)) {
        return 1; // Exit if Java installation fails
    }

    // Check for and download JBomb Launcher
    if (!checkAndDownloadJBombLauncher(jbombJar, jbombJarUrl)) {
        return 1; // Exit if JBomb Launcher download fails
    }

    std::cout << "Launching JBomb Launcher..." << std::endl;
    launchGame("java -jar \"" + jbombJar + "\"");

    return 0;
}
