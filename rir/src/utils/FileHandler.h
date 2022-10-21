#pragma once
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <unordered_map>

//
// During the serialization phase we might create or append to existing metadata,
// if both of these operations happen in the same run, then sometimes the created
// file may not be found, leading to us only serializing a later compilation.
// This can lead to a case where we are capturing only generic compilations.
// This problem is system dependent and mostly random, current solution is to keep
// all the file descriptors open and let the OS close it automatically when the
// process exits
//
class FileHandler {
    private:
        static std::unordered_map<std::string, FILE *> openFiles;
    public:
        static bool fileAlreadyOpen(std::string & path);
        static bool fileExists(std::string & path);
        static FILE * getFileIfExists(std::string & path);
        static FILE * createOrGetFileForWriting(std::string & path);
};
