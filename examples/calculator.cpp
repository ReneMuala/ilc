#include "../include/ilc.hpp"
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <regex>
#include <tuple>

using namespace ILC;

void configure_cfg(ILC::context_free_grammar::context_free_grammar_t &cfg) {
  cfg.terminals = {
      PLUS,
      MINUS,
      TIMES,
      SLASH,
      EQU,
      DEQU,
      NOT,
      AND,
      OR,
      //
      NUM,
      ID
  };

  cfg.non_terminals = {
      STAT,
      // operations
      SUM,
      SUB,
      MUL,
      DIV,
      ASGN,
      EQLT,
      NEG,
      CONJ,
      DISJ,
      VAL,
  };

  cfg.start = STAT;

  /* clang-format off */
  cfg.production_rule = {
      production{STAT, {STAT, STAT}},
      production{STAT, {ASGN}},
      production{ASGN, {ID, EQU, VAL}},
      production{EQLT, {VAL, DEQU, VAL}},

      production{SUM, {VAL, PLUS, VAL}},
      production{SUB, {VAL, MINUS, VAL}},
      production{MUL, {VAL, TIMES, VAL}},
      production{DIV, {VAL, SLASH, VAL}},

      production{DISJ, {VAL, OR, VAL}},
      production{CONJ, {VAL, AND, VAL}},
      production{NEG,  {NOT, VAL}},
      production{VAL, {PARETHESIS_OPEN, VAL, PARETHESIS_CLOSE}},
      production{VAL, {NUM}},
      production{VAL, {EQLT}},
      production{VAL, {SUM}},
      production{VAL, {SUB}},
      production{VAL, {MUL}},
      production{VAL, {DIV}},
      production{VAL, {DISJ}},
      production{VAL, {CONJ}},
      production{VAL, {NEG}},
      production{VAL, {ID}},
  };
  /* clang-format on */

  cfg.verify();
}

tuple<vector<symbol_t>, vector<string>>
get_sentence_elements(const string sentence) {
  const auto sentence_splited = tokenization::split(regex(" "), sentence);

  const auto sentence_detached =
      tokenization::detach(regex(R"(\w+|\+|=+|.+)"), sentence_splited);

  /* clang-format off */
  const auto dictionary_symbol = list<pair<symbol_t, regex>>{
      {DEQU, regex(R"(==)")},
      {EQU, regex(R"(=)")},
      {TIMES, regex(R"(\*)")},
      {SLASH, regex(R"(/)")},
      {PLUS, regex(R"(\+)")},
      {MINUS, regex(R"(-)")},
      {NUM, regex(R"(\d+(\.\d+)?)")},
      {ID, regex(R"(\w+)")},
  };
      /* clang-format on */
      const auto tokens = tokenization::tokenize<symbol_t>(
      dictionary_symbol, ERROR, sentence_detached);
  auto sentence_symbols = vector<symbol_t>{};
  auto sentence_values = vector<string>{};

  for (const auto [symbol, value] : tokens) {
    sentence_symbols.push_back(symbol);
    sentence_values.push_back(value);
    // cout << ILC::symbol::symbols_string[symbol];
  }
  return std::tuple(sentence_symbols, sentence_values);
}

// namespace ILC
int main() {
  auto cfg = context_free_grammar_t{};
  configure_cfg(cfg);
  shared_ptr<derivation_result_t> _result = make_shared<derivation_result_t>();
  const auto [sentence_symbols, sentence_values] = get_sentence_elements(R"(
        y = x
      )");
  print_sentence_vector(sentence_symbols);
  cout << endl;
  // cout << endl;
  auto history = sentences_history_t{};
  auto is_valid = derivate_recursively(cfg, sentence_symbols, history, _result);
  if (is_valid) {
    cout << "valid\n";
    auto tree = build_derivation_tree<std::string>(_result);
    int index = 0;
    tree->for_each_leaf([&](shared_ptr<derivation_tree_node_t<string>> node) {
      if (index < sentence_values.size())
        node->value = sentence_values[index++];
    });

    map<string, string> vars;

    tree->for_each_child([&](shared_ptr<derivation_tree_node_t<string>> node) {
      double lhs, rhs;
      switch (node->symbol) {
      case ILC::symbol::ASGN:
        vars[node->children.front()->get_first_value(true).value_or("_")] =
            node->children.back()->get_first_value(true).value_or("nil");
        break;
      case ILC::symbol::SUM:
        lhs = atof(node->children.front()->get_first_value(true)->c_str());
        rhs = atof(node->children.back()->get_first_value(true)->c_str());
        node->value = to_string(lhs + rhs);
        break;
      case ILC::symbol::SUB:
        lhs = atof(node->children.front()->get_first_value(true)->c_str());
        rhs = atof(node->children.back()->get_first_value(true)->c_str());
        node->value = to_string(lhs - rhs);
        break;
      case ILC::symbol::MUL:
        lhs = atof(node->children.front()->get_first_value(true)->c_str());
        rhs = atof(node->children.back()->get_first_value(true)->c_str());
        node->value = to_string(lhs * rhs);
        break;
      case ILC::symbol::DIV:
        lhs = atof(node->children.front()->get_first_value(true)->c_str());
        rhs = atof(node->children.back()->get_first_value(true)->c_str());
        node->value = to_string(lhs / rhs);
        break;
      }
    });
    cout << to_string(vars) << endl;
    tree->print(cout, {"(", ")"},
                [](const auto &symbol, std::ostream &ostream,
                   const optional<string> &value) {
                  cout << ILC::symbol::symbols_string[symbol];
                  //  << "{"
                  //      << (value ? value->c_str() : "nil") << "}";
                });
  } else {
    cout << "invalid\n";
  }
}
