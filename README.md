
![logo](images/ILC.png "ILC")

# ILC - I Love Compilers

ILC is a powerful, header-only C++20 framework designed for parsing, providing a set of libraries for tasks ranging from simple calculators to complex programming language syntax parsers. It leverages **context-free grammars** and employs a **bottom-up parsing** technique for efficient and effective parsing.

## Features

- **Simplicity**: ILC is designed to be straightforward and easy to use, making it accessible for both beginners and experienced developers.

- **Powerful Parsing**: The framework uses context-free grammars to successfully parse input, making it suitable for a variety of parsing tasks.

- **Header-Only**: ILC is a header-only library, making integration into your projects seamless without the need for additional build configurations.

## Getting Started

To get started with ILC, follow these simple steps:

1. **Include ILC Header**: Include the ILC header file in your project.

   ```cpp
   #include "ilc.hpp"
   ```

2. **Define Grammar Rules**: Specify the context-free grammar rules for your parser.

   ```cpp
    auto cfg = context_free_grammar_t{};

    cfg.terminals = {TIMES, PLUS, NUM};

    cfg.non_terminals = {S, A, B};

    cfg.start = S;

    cfg.production_rule = {
        // S -> S + A | A
        production{S, {S, PLUS, A}},  production{S, {A}},
        // A -> A * B | B
        production{A, {A, TIMES, B}}, production{A, {B}},
        // B -> {NUMBER} | S
        production{B, {NUM}},         production{B, {S}},
    };

    cfg.verify();
   ```

3. **Parse Input**: Use the provided functions to parse input according to your grammar.

   ```cpp
    shared_ptr<derivation_result_t> _result = make_shared<derivation_result_t>();
    // # + # * # + #
    auto sentence_symbols =
        vector<symbol_t>{NUM, PLUS, NUM, TIMES, NUM, PLUS, NUM};
    // used to avoid infinit loops
    auto history = sentences_history_t{};

    bool is_valid = derivate_recursively(cfg, sentence_symbols, history, _result);
   ```

4. **Check Results**: Verify the parsing result and utilize the provided tools to build a tree for further analysis.

   ```cpp
     if (is_valid) {
        cout << "valid\n";
        auto tree = build_derivation_tree<int>(_result);
        // (# + (# * #)) + #
        tree->print(cout);
    } else {
        cout << "invalid\n";
    }
   ```

## Example Usage

```cpp
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
```

## Contributing

We welcome contributions! If you have ideas for improvements or find any issues, please open an [issue](https://github.com/your-username/ilc/issues) or submit a [pull request](https://github.com/your-username/ilc/pulls).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


