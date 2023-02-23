#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <R/Protect.h>

#include "bc/BC_inc.h"
#include "R/r.h"
#include "interpreter/instance.h"
#include "utils/Pool.h"
class REnvHandler {
  public:
    REnvHandler(SEXP c) : _container(c) {
      assert(TYPEOF(c) == ENVSXP);
    }

    REnvHandler(uint32_t cPoolIndex) {
      SEXP c = rir::Pool::get(cPoolIndex);
      if (c == R_NilValue) {
        rir::Protect protecc;
        protecc(c = R_NewEnv(R_EmptyEnv,0,0));
        rir::Pool::patch(cPoolIndex, c);
      }
      assert(TYPEOF(c) == ENVSXP);
      _container = c;
    }

    // container
    SEXP container() {
      return _container;
    }

    // size
    int size() {
      int size = 0;
      iterate([&] (std::string key, SEXP val) {
        size++;
      });
      return size;
    }

    // empty check
    bool isEmpty() {
      bool empty = true;
      iterate([&] (std::string key, SEXP val) {
        empty = false;
      });
      return empty;

    }

    // removes a binding from the environment
    void remove(SEXP key) {
      R_removeVarFromFrame(key, _container);
    }

    void remove(const std::string & key) {
      remove(Rf_install(key.c_str()));
    }

    void remove(const char * key) {
      remove(Rf_install(key));
    }

    // setters: binds a key to a value
    void set(SEXP key, SEXP val) {
      assert(TYPEOF(key) == SYMSXP);
      Rf_defineVar(key, val, _container);
    }

    void set(const std::string & key, SEXP val) {
      set(Rf_install(key.c_str()), val);
    }

    void set(const char * key, SEXP val) {
      set(Rf_install(key), val);
    }

    // getters: returns nullptr if the key does not exist
    SEXP get(SEXP key) {
      assert(TYPEOF(key) == SYMSXP);
      SEXP binding = Rf_findVarInFrame(_container, key);
      return (binding == R_UnboundValue) ? nullptr : binding;
    }

    SEXP get(const std::string & key) {
      return get(Rf_install(key.c_str()));
    }

    SEXP get(const char * key) {
      return get(Rf_install(key));
    }

    SEXP operator[](SEXP key) {
      return get(key);
    }

    SEXP operator[](const std::string & key) {
      return get(Rf_install(key.c_str()));
    }

    SEXP operator[](const char * key) {
      return get(Rf_install(key));
    }

    // iterator
    void iterate(const std::function< void(std::string, SEXP) >& callback) {
      rir::Protect protecc;
      SEXP offsetBindings = R_lsInternal(_container, (Rboolean) false);
      protecc(offsetBindings);
      for (int i = 0; i < Rf_length(offsetBindings); i++) {
        // offsetBindings = R_lsInternal(_container, (Rboolean) false);
        std::string key = CHAR(STRING_ELT(offsetBindings, i));
        SEXP binding = Rf_findVarInFrame(_container, Rf_install(CHAR(STRING_ELT(offsetBindings, i))));
        // skip inactive bindings
        if (binding == R_UnboundValue) continue;
        callback(key, binding);
      }
    }
  private:
    SEXP _container;
};
