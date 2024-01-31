#include <algorithm>
#include <array>
#include <deque>
#include <exception>
#include <iostream>
#include <vector>
using namespace std;

enum symbol_t { VAL, ID, NUM, PLUS, EQU };

const string symbols_string[] = {"VAL", "ID", "NUM", "PLUS", "EQU"};

struct derivation_result_t {
  vector<symbol_t> old_chain, new_chain;
};

struct derivation_history_unit_t {
  int production_index;
  vector<symbol_t> chain;
};

inline bool operator==(vector<symbol_t> A, vector<symbol_t> B) {
  return equal(A.begin(), A.end(), B.begin(), B.end());
}

bool check_derivation_history(deque<derivation_history_unit_t> &history,
                              const int &production_index,
                              const vector<symbol_t> &chain) {
  bool found = find_if(history.begin(), history.end(),
                       [&](const derivation_history_unit_t &unit) {
                         return unit.production_index == production_index and
                                unit.chain == chain;
                       }) != history.end();
  if (not found)
    history.push_back(derivation_history_unit_t{
        .production_index = production_index, .chain = chain});
  return found;
}

void print_chain(vector<symbol_t> &chain) {
  for (const auto &symbol : chain) {
    cout << symbols_string[symbol] << " ";
  }
  cout << endl << endl;
}

bool _handle_terminal(deque<derivation_history_unit_t> &history,
                      vector<symbol_t> &chain, derivation_result_t &result,
                      const symbol_t symbol) {
  try {
    bool matches = chain[0] == symbol;
    if (matches) {
      result.old_chain = chain;
      chain.erase(chain.begin(), chain.begin() + 1);
      result.new_chain = chain;
      return true;
    }
  } catch (runtime_error &e) {
    cerr << e.what();
  }
  return false;
}

// VAL -> NUM
bool _handle_val__num(deque<derivation_history_unit_t> &history,
                      vector<symbol_t> &chain, derivation_result_t &result) {
  cout << "VAL -> NUM\n";
  print_chain(chain);
  try {
    const vector<symbol_t> dchain = {NUM};
    bool matches = equal(dchain.begin(), dchain.end(), chain.begin(),
                         chain.begin() + dchain.size());
    // if (check_derivation_history(history, 3, chain))
    //   return false;
    if (matches) {
      result.old_chain = chain;
      chain.erase(chain.begin(), chain.begin() + dchain.size());
      result.new_chain = chain;
      return true;
    }
  } catch (runtime_error &e) {
    cerr << e.what();
  }
  return false;
}

// VAL -> VAL PLUS VAL
bool _handle_val__val_plus_val(deque<derivation_history_unit_t> &history,
                               vector<symbol_t> &chain,
                               derivation_result_t &result) {
  cout << "VAL -> VAL PLUS VAL\n";
  print_chain(chain);
  try {
    const symbol_t dchain[] = {VAL, PLUS, VAL};
    const auto dchain_size = sizeof(dchain) / sizeof(symbol_t);
    // dchain has more elements than chain cancel
    if (dchain_size > chain.size())
      return false;
    if (check_derivation_history(history, 2, chain))
      return false;
    derivation_result_t nested_results[dchain_size];
    const auto chain_cpy = chain;
    bool matches = true;
    matches = matches and
              (_handle_val__val_plus_val(history, chain, nested_results[0]) or
               _handle_val__num(history, chain, nested_results[0]));
    matches = matches and
              _handle_terminal(history, chain, nested_results[1], dchain[1]);
    matches = matches and
              (_handle_val__val_plus_val(history, chain, nested_results[2]) or
               _handle_val__num(history, chain, nested_results[2]));
    return matches;

  } catch (runtime_error &e) {
    cerr << e.what();
  }
  return false;
}

// ASGN -> ID EQU VAL
bool _handle_asgn__id_equ_val(deque<derivation_history_unit_t> &history,
                              vector<symbol_t> &chain,
                              derivation_result_t &result) {
  try {
    cout << "ASGN -> ID EQU VAL\n";
    print_chain(chain);
    if (check_derivation_history(history, 1, chain))
      return false;
    const symbol_t dchain[] = {ID, EQU, VAL};
    derivation_result_t nested_results[sizeof(dchain) / sizeof(symbol_t)];
    const auto chain_cpy = chain;
    bool matches = true;
    matches = _handle_terminal(history, chain, nested_results[0], dchain[0]);
    matches = matches and
              _handle_terminal(history, chain, nested_results[1], dchain[1]);
    matches = matches and
              (_handle_val__val_plus_val(history, chain, nested_results[2]) or
               _handle_val__num(history, chain, nested_results[2]));
    return matches;
  } catch (runtime_error &e) {
    cerr << e.what();
  }
  return false;
}

bool _parse(deque<derivation_history_unit_t> &history, vector<symbol_t> &chain,
            derivation_result_t &result) {
  return _handle_asgn__id_equ_val(history, chain, result) && chain.empty();
}

int main() {
  vector<symbol_t> chain = {ID, EQU, NUM, PLUS, NUM, PLUS, NUM};
  deque<derivation_history_unit_t> history;
  derivation_result_t result{};
  bool is_valid = _parse(history, chain, result);
  cout << (is_valid ? "is valid" : "is not valid") << endl;
}
