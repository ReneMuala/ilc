#include "../include/ilc.hpp"

// namespace ILC
int main() {
  using namespace ILC;
  auto cfg = context_free_grammar_t{};
  cfg.terminals = {TIMES, PLUS, NUM};

  cfg.non_terminals = {S, A, B};

  cfg.start = S;

  cfg.production_rule = {
      production{S, {S, PLUS, A}},  production{S, {A}},
      production{A, {A, TIMES, B}}, production{A, {B}},
      production{B, {NUM}},         production{B, {S}},
  };

  cfg.verify();

  shared_ptr<derivation_result_t> _result = make_shared<derivation_result_t>();

  auto sentence_symbols =
      vector<symbol_t>{NUM, PLUS, NUM, TIMES, NUM, PLUS, NUM};
  auto history = sentences_history_t{};
  auto is_valid = derivate_recursively(cfg, sentence_symbols, history, _result);
  if (is_valid) {
    cout << "valid\n";
    auto tree = build_derivation_tree<int>(_result);
    tree->print(cout);
  } else {
    cout << "invalid\n";
  }
}
