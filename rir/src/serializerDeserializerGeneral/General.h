#pragma once

#define SERIALIZER_ENABLED 1

#include "R/r.h"
#include <unordered_map>

//
// Spaces reserved in the Constant pool for the following
//

class GeneralMaps {
    public:
    // Mapping from a pool entry to hast and offset index
    static std::unordered_map<unsigned, std::pair<SEXP, unsigned>> SPOOL_HAST_MAP;
    static std::unordered_map<unsigned, std::pair<SEXP, unsigned>> CPOOL_HAST_MAP;

    // Mapping from a hast to a dispatch table container
    static std::unordered_map<SEXP, SEXP> HAST_VTAB_MAP;

    // Mapping from a hast to a CLOSXP
    static std::unordered_map<SEXP, SEXP> HAST_CLOS_MAP;

    // Mapping from a hast to a bool (blacklisted if true)
    static std::unordered_map<SEXP, bool> BL_MAP;

};

class GeneralFlags {
    public:
    static bool contextualCompilationSkip;
};
