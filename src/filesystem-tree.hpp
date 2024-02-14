#ifndef FILESYSTEM_TREE_HPP
#define FILESYSTEM_TREE_HPP
#include <iostream>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iomanip>
#include <functional>
#include <regex>
#include <queue>
#include <utility> //std::pair

namespace fs = std::filesystem;

// Define the Node structure
struct Node {
    std::string name;
    std::string fullPath;
    bool isFile;
    std::unordered_map<std::string, Node*> children;

    // Constructor
    Node(const std::string& n, const std::string& fullPath_, bool isF = false) : name(n), fullPath(fullPath_), isFile(isF) {}

    // Destructor
    ~Node() {
        for (auto& child : children) {
            delete child.second;
        }
    }

    
    // Function to print contents of a file as hexadecimal and string
    std::pair<std::stringstream, std::stringstream> getFileContents(uint32_t maxBytesToRead = 0) {
        std::ifstream file(fullPath, std::ios::binary);
        std::stringstream hexStream;
        std::stringstream stringStream;

        if (file) {
            char c;
            int bytesCounter = 0;
            while (file.get(c) && bytesCounter < maxBytesToRead) {
                if (bytesCounter % 16 == 0) {
                    hexStream << std::hex << std::setw(8) << std::setfill('0') << bytesCounter << " ";
                }

                // Print as hexadecimal
                hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c << " ";

                // Print as string
                if (isprint(c))
                    if(c == ' ')
                        stringStream << "…";
                    else 
                        stringStream << c;
                else if (c == '\0')
                    stringStream << "º"; // Substitute null character with 'º'
                else if (c == '\r')
                    stringStream << "\\r"; // Substitute null character with 'º'
                else if (c == '\n')
                    stringStream << "\\n"; // Substitute null character with 'º'
                else
                    stringStream << "•"; // Substitute non-printable characters with '•'
                
                bytesCounter++;
                if (bytesCounter % 16 == 0) {
                    hexStream << std::endl;
                }
            }
            // std::cout << hexStream.str() << std::endl;
            // std::cout << "\nFile Content as String:\n" << ANSI_COLOR_DARK_GREEN << stringStream.str() << ANSI_COLOR_RESET << std::endl;
            file.close();
        } else {
            std::cerr << "Unable to open file: " << fullPath << std::endl;
        }
        return std::make_pair(std::move(hexStream), std::move(stringStream));
    }
};

// File System Tree class
class FileSystemTree {
public:
    Node* root;
private:
    std::regex includeFilter;
    std::regex excludeFilter;

    const std::string ANSI_COLOR_ORANGE = "\033[38;5;208m";
    const std::string ANSI_COLOR_DARK_GREY = "\033[38;5;239m";
    const std::string ANSI_COLOR_DARK_GREEN = "\033[38;5;22m";
    const std::string ANSI_COLOR_RESET = "\033[0m";

public:
    // Constructor with options for printing file contents, max bytes to read, max characters to show, include regex, and exclude regex
    FileSystemTree(const std::string& includeRegex = "", const std::string& excludeRegex = "") : 
    root(new Node("root", "/")), includeFilter(includeRegex.empty() ? ".*" : includeRegex), excludeFilter(excludeRegex) {}

    // Destructor
    ~FileSystemTree() {
        delete root;
    }

    // Function to add a file or directory
    void add(const std::string& path, bool isFile) {
        std::vector<std::string> tokens;
        tokenize(path, tokens);

        Node* current = root;
        std::string currentPath = "/";
        for (const auto& token : tokens) {
            currentPath += token + "/";
            if (current->children.find(token) == current->children.end()) {
                current->children[token] = new Node(token, currentPath);
            }
            current = current->children[token];
        }
        current->isFile = isFile;
    }

    // Function to tokenize a path
    void tokenize(const std::string& path, std::vector<std::string>& tokens) {
        std::string token;
        for (char c : path) {
            if (c == '/') {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    // Function to construct the file system tree from a given directory
    void constructFromFilesystem(const std::string& directoryPath) {
        std::queue<std::pair<Node*, std::string>> queue;
        queue.push({root, directoryPath});

        while (!queue.empty()) {
            auto [currentNode, currentPath] = queue.front();
            queue.pop();

            for (const auto& entry : fs::directory_iterator(currentPath)) {
                std::string newPath = entry.path().string();
                bool isFile = entry.is_regular_file();
                std::string relativePath = entry.path().filename().string();

                // Check if the file or directory matches the inclusion filter
                bool includeMatch = std::regex_match(relativePath, includeFilter);

                // Check if the file or directory matches the exclusion filter
                bool excludeMatch = std::regex_match(relativePath, excludeFilter);

                if (includeMatch && !excludeMatch) {
                    currentNode->children[relativePath] = new Node(relativePath, entry.path().string(), isFile);
                    if (!isFile) {
                        queue.push({currentNode->children[relativePath], newPath});
                    }
                }
            }
        }
    }

    // Function to display the file system tree
    void display(Node* node, bool printFileData = false, uint32_t maxBytesToShow = 0, uint32_t depth = 0) {
        if (node == nullptr) {
            return;
        }

        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }

        std::cout << "|_" << node->name;
        if (node->isFile && printFileData) {
            std::cout << " [File]";
            std::cout << "\n Contents:\n";
            auto streams = node->getFileContents( maxBytesToShow);
            std::cout << streams.first.str() << std::endl;
            std::cout << streams.second.str() << std::endl;
            
        } else if (node->isFile) {
            std::cout << " [File]";
        } else {
            std::cout << " [Directory]";
        }
        std::cout << std::endl;

        for (auto& child : node->children) {
            display(child.second, printFileData, maxBytesToShow, depth + 1);
        }
    }

    // Wrapper function to display the file system tree
    void displayFileSystem(bool printFileContents= false, uint32_t maxBytesToShow=0) {
        display(root, printFileContents, maxBytesToShow);
    }

};

#if 0
int main() {
    // Provide the path to the parent folder
    std::string parentFolderPath = "/home/";

    // Create FileSystemTree object with printContents option enabled, maximum bytes to read, and maximum characters to show
    FileSystemTree fileSystem(true, 100, 50); // Will read only first 100 bytes of each file and show only first 50 characters of interpreted string content

    // Construct the file system tree from the given directory
    fileSystem.constructFromFilesystem(parentFolderPath);

    // Displaying the file system tree
    fileSystem.displayFileSystem();

    // Example usage of forEachNode function
    std::cout << "\nIterating over each node in the file system tree:\n";
    fileSystem.forEachNode([](Node* node) {
        std::cout << "Name: " << node->name << ", Full Path: " << node->fullPath << std::endl;
    });

    return 0;
}
#endif

#endif