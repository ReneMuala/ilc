#pragma once
#include <functional>
#include <vector>

struct derivation_history_t {
  int offset{-1};
};

inline bool CHECK_DERIVATION_HISTORY(derivation_history_t &history);
inline void CLEAR_DERIVATION_HISTORY(derivation_history_t &history);

inline bool HANDLE_TERMINAL(int);               // definined with
                                                // BEGIN_BINDINGS
constexpr inline bool HANDLE_NON_TERMINAL(int); // definined with BEGIN_BINDINGS

#define END_ILC_CODEGEN

#define BEGIN_ILC_CODEGEN                                                      \
  namespace ILC {                                                              \
  static derivation_history_t history{};                                       \
  static std::vector<SYMBOL> chain{};                                          \
  static size_t chain_size{0};                                                 \
  static int offset{0};                                                        \
  }

#if defined(BEGIN_ILC_CODEGEN)

#define BEGIN_BINDINGS                                                         \
  constexpr inline bool HANDLE_NON_TERMINAL(int _SYMBOL) {                     \
    switch (_SYMBOL) {

#define BEGIN_SYMBOL_BINDING(S)                                                \
  case S:                                                                      \
    return
#define END_SYMBOL_BINDING ;

#define END_BINDINGS                                                           \
  default:                                                                     \
    return false;                                                              \
    }                                                                          \
    }                                                                          \
    inline bool HANDLE_TERMINAL(int symbol) {                                  \
      if (ILC::offset < ILC::chain_size and                                    \
          ILC::chain[ILC::offset] == symbol) {                                 \
        ILC::offset++;                                                         \
        return true;                                                           \
      }                                                                        \
      return false;                                                            \
    }                                                                          \
    inline bool CHECK_DERIVATION_HISTORY(derivation_history_t &history) {      \
      static bool found;                                                       \
      found = history.offset >= ILC::offset;                                   \
      if (not found)                                                           \
        history.offset = ILC::offset;                                          \
      return found;                                                            \
    }                                                                          \
    inline void CLEAR_DERIVATION_HISTORY(derivation_history_t &history) {      \
      history.offset = -1;                                                     \
    }

#define DECLARE_PRODUCTION(NAME) inline bool NAME();

#define SET_CALLBACK(C)                                                        \
  std::function<void()> CALLBACK = C;                                          \
  CALLBACK();

#define BEGIN_PRODUCTION(NAME)                                                 \
                                                                               \
  inline bool NAME() {                                                         \
    static derivation_history_t HISTORY;                                       \
    if (CHECK_DERIVATION_HISTORY(HISTORY))                                     \
      return false;                                                            \
    const size_t offsetc = ILC::offset;

#define END_PRODUCTION                                                         \
  CLEAR_DERIVATION_HISTORY(HISTORY);                                           \
  return true;                                                                 \
  }

#define BEGIN_CHAIN_DECLARATION static const SYMBOL DECLARED_CHAIN[] = {
#define END_CHAIN_DECLARATION                                                  \
  }                                                                            \
  ;                                                                            \
  static const auto DECLARED_CHAIN_SIZE =                                      \
      sizeof(DECLARED_CHAIN) / sizeof(SYMBOL);

#define REQUIRE_TERMINAL(I)                                                    \
  if (not(HANDLE_TERMINAL(DECLARED_CHAIN[I]))) {                               \
    ILC::offset = offsetc;                                                     \
    return false;                                                              \
  }
#define REQUIRE_NON_TERMINAL(I)                                                \
  if (not(HANDLE_NON_TERMINAL(DECLARED_CHAIN[I]))) {                           \
    ILC::offset = offsetc;                                                     \
    return false;                                                              \
  }

#endif