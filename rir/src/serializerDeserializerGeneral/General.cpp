#include "General.h"

std::unordered_map<unsigned, std::pair<SEXP, unsigned>> GeneralMaps::SPOOL_HAST_MAP;
std::unordered_map<unsigned, std::pair<SEXP, unsigned>> GeneralMaps::CPOOL_HAST_MAP;
std::unordered_map<SEXP, SEXP> GeneralMaps::HAST_VTAB_MAP;
std::unordered_map<SEXP, SEXP> GeneralMaps::HAST_CLOS_MAP;
std::unordered_map<SEXP, bool> GeneralMaps::BL_MAP;


bool GeneralFlags::contextualCompilationSkip = getenv("SKIP_CONTEXTUAL_COMPILATION") ? true : false;
