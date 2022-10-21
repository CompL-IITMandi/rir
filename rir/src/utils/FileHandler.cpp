#include "utils/FileHandler.h"

std::unordered_map<std::string, FILE *> FileHandler::openFiles;

bool FileHandler::fileAlreadyOpen(std::string & path) {
    return openFiles.find(path) != openFiles.end();
}

bool FileHandler::fileExists(std::string & path) {
    if (access(path.c_str(), F_OK) == 0) {
        return true;
    }
    return false;
}

FILE * FileHandler::getFileIfExists(std::string & path) {
    // If file is already open, rewind and send back
    if (fileAlreadyOpen(path)) {
        FILE* res = openFiles[path];
        rewind(res);
        return res;
    }

    // File is not already open, check if it is contained in the directory
    if (fileExists(path)) {
        // Open an already existing file for read/write
        FILE* fp = std::fopen(path.c_str(), "r+");

        if (fp == nullptr) {
            std::cerr << "Unexpected: File exists in directory but unable to open" << std::endl;
            return nullptr;
        }
        openFiles[path] = fp;
        return fp;
    }

    return nullptr;
}

FILE * FileHandler::createOrGetFileForWriting(std::string & path) {
    // If file is already open, rewind and send back
    if (fileAlreadyOpen(path)) {
        FILE* res = openFiles[path];
        rewind(res);
        return res;
    }

    FILE* fp = std::fopen(path.c_str(), "w+");

    if (fp == nullptr) {
        std::cerr << "Unexpected: w+ failed!!" << std::endl;
        return nullptr;
    }
    openFiles[path] = fp;
    return fp;
}
