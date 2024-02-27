#include "../../../include/ilc.hpp"
#include "jit/jit-common.h"
#include "jit/jit-context.h"
#include "jit/jit-function.h"
#include "jit/jit-insn.h"
#include "jit/jit-type.h"
#include "jit/jit-util.h"
#include "jit/jit-value.h"
#include "libjit/include/jit/jit.h"
#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <iterator>
#include <regex>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
enum SYMBOL {
  PARENTHESIS_OPEN,
  PARENTHESIS_CLOSE,
  KEY_INICIO,
  KEY_FIM,
  KEY_ESCREVER,
  KEY_LER,
  KEY_PARA,
  KEY_ATE,
  KEY_REPETIR,
  KEY_SE,
  KEY_ENTAO,
  KEY_SENAO,
  KEY_ENQUANTO,
  STAT,
  SE_STAT,
  COMMA,
  TYPE_BOOLEANO,
  TYPE_REAL,
  TYPE_TEXT,
  VAR,
  VAL,
  SINGLE_VAL,
  SUFIXED_VAL,
  ID,

  NUM,
  TEXT,
  PLUS,
  MINUS,
  TIMES,
  SLASH,

  GREATER,
  GREATER_OR_EQUAL,
  LOWER,
  LOWER_OR_EQUAL,
  EQUAL,
  NOT_EQUAL,
  NOT,
  AND,
  OR,

  EQU,
  KEY_VERDADE,
  KEY_FALSO,
  ERROR
};

typedef std::variant<float, std::string> value_type;

bool se_context_enabled = true;
std::stack<value_type> values;
std::vector<std::string> str_chain;
std::map<std::string, value_type> variables;

namespace serra_jit {
jit_type_t type_cstring = jit_type_create_pointer(jit_type_sys_char, 1);
jit_context_t context;
jit_function_t function;
std::stack<jit_value_t> values;
std::vector<std::string> string_constants;
std::map<std::string, jit_value_t> variables;
std::vector<std::string> input_data;
} // namespace serra_jit

extern "C" {
void print_str(void *value) {
  const char *value_str = (const char *)value;
  std::cout << (value_str ? value_str : "<nada>");
}

void print_float(float value) { std::cerr << value; }

void print_boolean(int value) { std::cout << (value ? "verdade" : "falso"); }

void *read_str() {
  std::string input;
  std::cin >> input;
  serra_jit::input_data.push_back(std::move(input));
  return (void *)serra_jit::input_data.at(serra_jit::input_data.size() - 1)
      .c_str();
}

float read_float() {
  float input;
  std::cin >> input;
  return input;
}

int read_boolean() {
  int input;
  std::cin >> input;
  return input;
}
}

BEGIN_ILC_CODEGEN

/* clang-format off */

BEGIN_PRODUCTION(PROGRAM)
BEGIN_CHAIN_DECLARATION KEY_INICIO, STAT, KEY_FIM END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  while(TRY_REQUIRE_NON_TERMINAL(1));
  if(not (TRY_REQUIRE_TERMINAL(2) and ILC::offset == ILC::chain_size)){
    std::cerr << "A compilacao foi interrompida antes de ("<< ILC::offset << "): ";
    for(int i = ILC::offset; i < ILC::offset + 5 and i < ILC::chain_size; i++)
      std::cerr << str_chain[i] << " ";
    std::cerr << std::endl;
  }
END_PRODUCTION

BEGIN_PRODUCTION(SE_DEF)
BEGIN_CHAIN_DECLARATION KEY_SE, VAL, KEY_ENTAO, STAT, KEY_FIM, KEY_SENAO, SE_STAT END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  REQUIRE_NON_TERMINAL(1)
  REQUIRE_TERMINAL(2)
  jit_label_t label_if_not, label_end;
  label_if_not = label_end = jit_label_undefined;
  const auto top = serra_jit::values.top();
  serra_jit::values.pop();
  if(jit_value_get_type(top) == serra_jit::type_cstring){
    std::cerr << "apenas numeros e booleanos podem ser usados em condicoes" << std::endl;
    exit(-1);
  }
  jit_insn_branch_if_not(serra_jit::function, top, &label_if_not);
  while(TRY_REQUIRE_NON_TERMINAL(3));
  jit_insn_label(serra_jit::function, &label_if_not);
  if(TRY_REQUIRE_TERMINAL(5)){
    if(not TRY_REQUIRE_NON_TERMINAL(6)){
      jit_insn_branch_if(serra_jit::function, top, &label_end);
      while(TRY_REQUIRE_NON_TERMINAL(3));
      jit_insn_label(serra_jit::function, &label_end);
      REQUIRE_TERMINAL(4)
    }
  } else {
    jit_insn_label(serra_jit::function, &label_end);
    REQUIRE_TERMINAL(4)
  }
END_PRODUCTION

BEGIN_PRODUCTION(PARA_DEF)
BEGIN_CHAIN_DECLARATION KEY_PARA, ID, EQU, VAL, KEY_ATE, VAL, COMMA, VAL, KEY_REPETIR, STAT, KEY_FIM END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  REQUIRE_TERMINAL(1)
  const std::string var_def_name = str_chain[ILC::offset-1];
  if(variables.count(var_def_name) > 0){
      std::cerr << "A variavel '" << var_def_name << "' esta sendo redeclarada no loop 'para'.\n";
      exit(-1);
  }
  REQUIRE_TERMINAL(2)
  REQUIRE_NON_TERMINAL(3)
  
  // BEGIN
  auto top = serra_jit::values.top();
  if(jit_value_get_type(top) != jit_type_float32){
      std::cerr << "Loops 'para' devem fazer uso de valores numericos\n";
      exit(-1);
  }
  auto begin_val = jit_value_create(serra_jit::function, jit_type_float32);
  jit_insn_store(serra_jit::function, begin_val, top);
  serra_jit::variables[var_def_name] = begin_val;
  serra_jit::values.pop();

  REQUIRE_TERMINAL(4)
  REQUIRE_NON_TERMINAL(5)

  // END
  top = serra_jit::values.top();
  serra_jit::values.pop();
  if(jit_value_get_type(top) != jit_type_float32){
      std::cerr << "Loops 'para' devem fazer uso de valores numericos\n";
      exit(-1);
  }
  auto end_val = top;

  jit_value_t increment;
  // INCREMENT
  if(TRY_REQUIRE_TERMINAL(6)){
    REQUIRE_NON_TERMINAL(7)
    top = serra_jit::values.top();
    serra_jit::values.pop();
    if(jit_value_get_type(top) != jit_type_float32){
        std::cerr << "Loops 'para' devem fazer uso de valores numericos\n";
        exit(-1);
    }
    increment = top;
  } else {
    increment =  jit_value_create_float32_constant(serra_jit::function, jit_type_float32, 1);
  }
  
  REQUIRE_TERMINAL(8)
  
  jit_label_t loop_begin, loop_end;
  loop_begin = loop_end = jit_label_undefined;
  jit_insn_label(serra_jit::function, &loop_begin);
  jit_insn_branch_if(serra_jit::function,
    jit_insn_eq(serra_jit::function, begin_val, end_val), &loop_end);
  while(TRY_REQUIRE_NON_TERMINAL(9));
  jit_insn_store(serra_jit::function, begin_val, 
    jit_insn_add(serra_jit::function, begin_val, increment));
  jit_insn_branch(serra_jit::function, &loop_begin);
  jit_insn_label(serra_jit::function, &loop_end);
  REQUIRE_TERMINAL(10)
  serra_jit::variables.erase(var_def_name);
END_PRODUCTION

BEGIN_PRODUCTION(ESCREVER_INSTRUCTION)
BEGIN_CHAIN_DECLARATION KEY_ESCREVER, VAL, COMMA END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  static jit_value_t space = jit_value_create_long_constant(
      serra_jit::function, serra_jit::type_cstring, (long)" ");
  static jit_function_t print_str_jit_func = jit_function_from_closure(serra_jit::context, (void*)print_str);
  static jit_type_t print_str_params[1] = {serra_jit::type_cstring};
  static jit_type_t print_str_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, print_str_params, 1, 1);
  
  static jit_function_t print_float_jit_func = jit_function_from_closure(serra_jit::context, (void*)print_float);
  static jit_type_t print_float_params[1] = {jit_type_float32};
  static jit_type_t print_float_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, print_float_params, 1, 1);
  
  static jit_function_t print_boolean_jit_func = jit_function_from_closure(serra_jit::context, (void*)print_boolean);
  static jit_type_t print_boolean_params[1] = {jit_type_int};
  static jit_type_t print_boolean_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, print_boolean_params, 1, 1);

  bool first = true;
  jit_value_t temp_args[1];
  do {
    REQUIRE_NON_TERMINAL(1)
    if(not first) {
      temp_args[0] = space;
      jit_insn_call_native(serra_jit::function, "print_str", (void*)print_str, print_str_signature, temp_args, 1, 0);
    };
    auto value = serra_jit::values.top();
    if(jit_value_get_type(value) == jit_type_float32){
      temp_args[0] = value;
      jit_insn_call_native(serra_jit::function, "print_float", (void*)print_float, print_float_signature, temp_args, 1, 0);
    } else if(jit_value_get_type(value) == serra_jit::type_cstring){
      temp_args[0] = value;
      jit_insn_call_native(serra_jit::function, "print_str", (void*)print_str, print_str_signature, temp_args, 1, 0);
    } else if(jit_value_get_type(value) == jit_type_int){
      temp_args[0] = value;
      jit_insn_call_native(serra_jit::function, "print_boolean", (void*)print_boolean, print_boolean_signature, temp_args, 1, 0);
    }
    if(first)
      first = false;
  } while(TRY_REQUIRE_TERMINAL(2));
END_PRODUCTION

BEGIN_PRODUCTION(LER_INSTRUCTION)
BEGIN_CHAIN_DECLARATION KEY_LER, ID, COMMA END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  static jit_function_t read_str_jit_func = jit_function_from_closure(serra_jit::context, (void*)read_str);
  static jit_type_t read_str_params[0];
  static jit_type_t read_str_signature = jit_type_create_signature(jit_abi_cdecl, serra_jit::type_cstring, read_str_params, 0, 1);
  
  static jit_function_t read_float_jit_func = jit_function_from_closure(serra_jit::context, (void*)read_float);
  static jit_type_t read_float_params[0];
  static jit_type_t read_float_signature = jit_type_create_signature(jit_abi_cdecl, jit_type_float32, read_float_params, 0, 1);
  jit_value_t temp_args[0];
  do {
    REQUIRE_TERMINAL(1)
    const std::string id = str_chain[ILC::offset-1];
    try {
      auto & dest = serra_jit::variables.at(id);
      if(not se_context_enabled) continue;
      if(jit_value_get_type(dest) == jit_type_float32){
        jit_insn_store(serra_jit::function, dest, jit_insn_call_native(serra_jit::function, "read_float", (void*)read_float, read_float_signature, temp_args, 0, 0));
      } else if(jit_value_get_type(dest) == serra_jit::type_cstring){
        jit_insn_store(serra_jit::function, dest, jit_insn_call_native(serra_jit::function, "read_str", (void*)read_str, read_str_signature, temp_args, 0, 0));
      }
    } catch(std::exception & e){
      std::cerr << "A variavel '" << id <<"' nao existe" << std::endl;
      exit(-1);
    }
  }while(TRY_REQUIRE_TERMINAL(2));
END_PRODUCTION

std::vector<std::string> var_def_names;
BEGIN_PRODUCTION(PREFIX_VAR_DEF)
BEGIN_CHAIN_DECLARATION VAR, ID, COMMA END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  REQUIRE_TERMINAL(1)
  var_def_names.clear();
  var_def_names.push_back(str_chain[ILC::offset-1]);
  while(TRY_REQUIRE_TERMINAL(2)) {
    REQUIRE_TERMINAL(1)
    var_def_names.push_back(str_chain[ILC::offset-1]);
  }
  SET_CALLBACK([]{
    if(not se_context_enabled) return;
    for (const auto & var_def_name : var_def_names) {  
      if(variables.count(var_def_name) > 0){
        std::cerr << "A variavel '" << var_def_name << "' esta sendo redeclarada.\n";
        exit(-1);
      }
    }
  })
END_PRODUCTION

BEGIN_PRODUCTION(VAR_TYPE_BOOLEANO)
BEGIN_CHAIN_DECLARATION TYPE_BOOLEANO END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  SET_CALLBACK([]{
    for (const auto & var_def_name : var_def_names) {  
      serra_jit::variables[var_def_name] = jit_value_create(serra_jit::function, jit_type_int);
    }
  })
END_PRODUCTION

BEGIN_PRODUCTION(VAR_TYPE_REAL)
BEGIN_CHAIN_DECLARATION TYPE_REAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  SET_CALLBACK([]{
    for (const auto & var_def_name : var_def_names) {  
      serra_jit::variables[var_def_name] = jit_value_create(serra_jit::function, jit_type_float32);
    }
  })
END_PRODUCTION

BEGIN_PRODUCTION(VAR_TYPE_TEXT)
BEGIN_CHAIN_DECLARATION TYPE_TEXT END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  SET_CALLBACK([]{
    for (const auto & var_def_name : var_def_names) {  
      serra_jit::variables[var_def_name] = jit_value_create(serra_jit::function, serra_jit::type_cstring);
    }
  })
END_PRODUCTION

BEGIN_PRODUCTION(PREFIX_VAL)
BEGIN_CHAIN_DECLARATION VAL END_CHAIN_DECLARATION 
  REQUIRE_NON_TERMINAL(0)
END_PRODUCTION

BEGIN_PRODUCTION(PLUS_VAL)
BEGIN_CHAIN_DECLARATION PLUS, VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A soma apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_add(serra_jit::function, lhs, rhs)
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(MINUS_VALUE)
BEGIN_CHAIN_DECLARATION MINUS, VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A subracao apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_sub(serra_jit::function, lhs, rhs)
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(TIMES_VALUE)
BEGIN_CHAIN_DECLARATION TIMES, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A multiplicacao apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_mul(serra_jit::function, lhs, rhs)
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(SLASH_VALUE)
BEGIN_CHAIN_DECLARATION SLASH, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A divisao apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_div(serra_jit::function, lhs, rhs)
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(GREATER_VALUE)
BEGIN_CHAIN_DECLARATION GREATER, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A comparacao (>) apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_gt(serra_jit::function, lhs, rhs)
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(GREATER_OR_EQUAL_VALUE)
BEGIN_CHAIN_DECLARATION GREATER_OR_EQUAL, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A comparacao (>=) apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_ge(serra_jit::function, lhs, rhs)
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(LOWER_VALUE)
BEGIN_CHAIN_DECLARATION LOWER, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A comparacao (<) apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_lt(serra_jit::function, lhs, rhs)
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(LOWER_OR_EQUAL_VALUE)
BEGIN_CHAIN_DECLARATION LOWER_OR_EQUAL, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A comparacao (<=) apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_le(serra_jit::function, lhs, rhs)
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(EQUAL_VALUE)
BEGIN_CHAIN_DECLARATION EQUAL, VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();

    if(jit_value_get_type(lhs) == serra_jit::type_cstring or jit_value_get_type(rhs) == serra_jit::type_cstring) {
      std::cerr << "A comparacao (==) apenas pode ser aplicada em numeros ou booleanos\n";
      exit(-1);
    }

    jit_strcmp(const char *str1, const char *str2)

    serra_jit::values.push(
      jit_insn_eq(serra_jit::function, lhs, rhs)
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(AND_EQUAL_VALUE)
BEGIN_CHAIN_DECLARATION AND, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(not(jit_value_get_type(lhs) == jit_type_float32 and jit_value_get_type(rhs) == jit_type_float32)) {
      std::cerr << "A comparacao (<=) apenas pode ser aplicada em numeros\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_and(serra_jit::function, lhs, rhs)
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(OR_VALUE)
BEGIN_CHAIN_DECLARATION OR, VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(jit_value_get_type(lhs) == serra_jit::type_cstring or jit_value_get_type(rhs) == serra_jit::type_cstring) {
      std::cerr << "A comparacao (==) apenas pode ser aplicada em numeros ou booleanos\n";
      exit(-1);
    }
    /*
    jit_insn_to_not_bool(serra_jit::function, 
        jit_insn_neg(serra_jit::function,
          jit_insn_to_bool(serra_jit::function,
            jit_insn_eq(serra_jit::function, lhs, rhs))))
    */
    serra_jit::values.push(
      jit_insn_or(serra_jit::function, lhs , rhs)
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(NOT_EQUAL_VALUE)
BEGIN_CHAIN_DECLARATION NOT_EQUAL, VAL, SINGLE_VAL, SUFIXED_VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  bool request_suffixed = false;
  if(not(request_suffixed = TRY_REQUIRE_NON_TERMINAL(2)))
    REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto rhs = serra_jit::values.top();
    serra_jit::values.pop();

    const auto lhs = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(jit_value_get_type(lhs) == serra_jit::type_cstring or jit_value_get_type(rhs) == serra_jit::type_cstring) {
      std::cerr << "A comparacao (!=) apenas pode ser aplicada em numeros ou booleanos\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_to_not_bool(serra_jit::function, 
        jit_insn_neg(serra_jit::function,
          jit_insn_to_bool(serra_jit::function,
            jit_insn_eq(serra_jit::function, lhs, rhs))))
    );
  })
  if(request_suffixed) REQUIRE_NON_TERMINAL(3)
END_PRODUCTION

BEGIN_PRODUCTION(VAL_AS_NUM)
BEGIN_CHAIN_DECLARATION NUM END_CHAIN_DECLARATION
  REQUIRE_TERMINAL(0)
  SET_CALLBACK([&]{
    serra_jit::values.push(
      jit_value_create_float32_constant(serra_jit::function, jit_type_float32, (float)atof(str_chain.at(ILC::offset-1).c_str()))
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(VAL_AS_TEXT)
BEGIN_CHAIN_DECLARATION TEXT END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  SET_CALLBACK([&]{
    std::string content = str_chain.at(ILC::offset-1);
    content = std::regex_replace(content, std::regex(R"(\\\")"), "\"");
    content = std::regex_replace(content, std::regex(R"(\\n)"), "\n");
    content = std::regex_replace(content, std::regex(R"(\\t)"), "\t");
    content = std::regex_replace(content, std::regex(R"(\\r)"), "\r");
    content = std::regex_replace(content, std::regex(R"(\\a)"), "\a");
    content = content.substr(1, content.size() - 2);
    jit_value_t bufptr = jit_value_create(serra_jit::function, serra_jit::type_cstring);
    jit_insn_store(serra_jit::function, bufptr, jit_insn_alloca(serra_jit::function, jit_value_create_nint_constant(serra_jit::function, jit_type_int, content.size()+1)));
    for(int i = 0; i < content.size(); i++){
      jit_insn_store_relative(serra_jit::function, bufptr, i, jit_value_create_nint_constant(serra_jit::function, jit_type_ubyte, content[i]));
    }
    jit_insn_store_relative(serra_jit::function, bufptr, content.size(), jit_value_create_nint_constant(serra_jit::function, jit_type_ubyte, 0));
    serra_jit::values.push(
      bufptr
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(NOT_VAL)
BEGIN_CHAIN_DECLARATION NOT, VAL, PARENTHESIS_CLOSE END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  REQUIRE_NON_TERMINAL(1)
  SET_CALLBACK([&](){
    const auto value = serra_jit::values.top();
    serra_jit::values.pop();
    
    if(jit_value_get_type(value) == serra_jit::type_cstring) {
      std::cerr << "A negacao (!=) apenas pode ser aplicada em numeros ou booleanos\n";
      exit(-1);
    }

    serra_jit::values.push(
      jit_insn_to_not_bool(serra_jit::function,
        jit_insn_neg(serra_jit::function,
          jit_insn_to_bool(serra_jit::function, value)))
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(VAL_WITH_PARENTHESIS)
BEGIN_CHAIN_DECLARATION PARENTHESIS_OPEN, VAL, PARENTHESIS_CLOSE END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  REQUIRE_NON_TERMINAL(1)
  REQUIRE_TERMINAL(2)
END_PRODUCTION

BEGIN_PRODUCTION(VAL_AS_ID)
BEGIN_CHAIN_DECLARATION ID END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  const std::string id = str_chain[ILC::offset-1];
  SET_CALLBACK([&]{
    try {
      serra_jit::values.push(
        serra_jit::variables.at(id)
      );
    } catch(std::exception & e){
      std::cerr << "A variavel '" << id <<"' nao existe :: " << ILC::offset - 1 << std::endl;
      exit(-1);
    }
  })
END_PRODUCTION

BEGIN_PRODUCTION(VAL_AS_VERDADE)
BEGIN_CHAIN_DECLARATION KEY_VERDADE END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  SET_CALLBACK([&]{
    serra_jit::values.push(
      jit_value_create_nint_constant(serra_jit::function, jit_type_int, 1)
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(VAL_AS_FALSO)
BEGIN_CHAIN_DECLARATION KEY_FALSO END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  SET_CALLBACK([&]{
    serra_jit::values.push(
      jit_value_create_nint_constant(serra_jit::function, jit_type_int, 0)
    );
  })
END_PRODUCTION

BEGIN_PRODUCTION(ASGN)
BEGIN_CHAIN_DECLARATION ID, EQU, VAL END_CHAIN_DECLARATION 
  REQUIRE_TERMINAL(0)
  const std::string id = str_chain[ILC::offset-1];
  REQUIRE_TERMINAL(1)
  REQUIRE_NON_TERMINAL(2)
  SET_CALLBACK([&]{
    auto & dest = serra_jit::variables.at(id);
    auto & src = serra_jit::values.top();
    serra_jit::values.pop();
    if(jit_value_get_type(dest) == jit_value_get_type(src)){
      jit_insn_store(serra_jit::function, dest, src);
    } else {
      std::cerr << " A variavel '" << id <<"' nao pode receber valores de um tipo diferente, em " << ILC::offset - 1 << std::endl;
      exit(-1);
    }
  })
END_PRODUCTION

BEGIN_BINDINGS
  BEGIN_SYMBOL_BINDING(STAT)
    (PREFIX_VAR_DEF() and (VAR_TYPE_BOOLEANO() or VAR_TYPE_REAL() or VAR_TYPE_TEXT()))
    or ASGN()
    or ESCREVER_INSTRUCTION()
    or LER_INSTRUCTION()
    or SE_DEF()
    or PARA_DEF()
  END_SYMBOL_BINDING
  BEGIN_SYMBOL_BINDING(VAL)
    (PREFIX_VAL() and (
      SLASH_VALUE()
      or TIMES_VALUE() 
      or PLUS_VAL() 
      or MINUS_VALUE()
      or GREATER_VALUE()
      or GREATER_OR_EQUAL_VALUE()
      or LOWER_VALUE()
      or LOWER_OR_EQUAL_VALUE()
      or EQUAL_VALUE()
      or NOT_EQUAL_VALUE()
      or true))
    or VAL_AS_NUM()
    or VAL_AS_TEXT() 
    or VAL_AS_ID()
    or VAL_AS_VERDADE()
    or VAL_AS_FALSO()
    or VAL_WITH_PARENTHESIS()
    or NOT_VAL()
  END_SYMBOL_BINDING
  BEGIN_SYMBOL_BINDING(SINGLE_VAL)
    VAL_AS_NUM()
    or VAL_AS_TEXT() 
    or VAL_AS_ID()
    or VAL_AS_VERDADE()
    or VAL_AS_FALSO()
    or VAL_WITH_PARENTHESIS()
    or NOT_VAL()
  END_SYMBOL_BINDING
    BEGIN_SYMBOL_BINDING(SUFIXED_VAL)
    SLASH_VALUE()
    or TIMES_VALUE() 
    or PLUS_VAL() 
    or MINUS_VALUE() 
    or GREATER_VALUE()
    or GREATER_OR_EQUAL_VALUE()
    or LOWER_VALUE()
    or LOWER_OR_EQUAL_VALUE()
    or EQUAL_VALUE()
    or NOT_EQUAL_VALUE()
    or true
  END_SYMBOL_BINDING
  BEGIN_SYMBOL_BINDING(SE_STAT)
    SE_DEF()
  END_SYMBOL_BINDING
END_BINDINGS

    /* clang-format on */
    END_ILC_CODEGEN

    using namespace ILC;

bool _parse() {
  compilation_id++;
  offset = 0;
  PROGRAM();
  return offset == chain_size;
}

/// use jumps and MINUS_VALlabels

std::tuple<std::vector<SYMBOL>, std::vector<std::string>>
get_sentence_elements(const std::string sentence) {
  using namespace ILC;
  // const auto sentence_splited = tokenization::split(std::regex(" "),
  // sentence);
  const std::regex rgx_space("\\s*");
  auto sentence_detached = tokenization::detach(
      std::regex(
          R"R(("(\\"|[^"])*")|\s+|\(|\)|\w+|,|&{2}|\|{2}|\+|-|\*|\/|(!=)|!|=+|(>=)|(<=)|<|>|.+)R"),
      sentence);
  sentence_detached = {sentence_detached.begin(),
                       std::remove_if(sentence_detached.begin(),
                                      sentence_detached.end(),
                                      [&rgx_space](std::string &str) {
                                        return std::regex_match(str, rgx_space);
                                      })};
  /* clang-format off */
  const auto dictionary_symbol = std::list<std::pair<SYMBOL,std::regex>>{
      {COMMA, std::regex(R"(,)")},
      {PARENTHESIS_OPEN, std::regex(R"(\()")},
      {PARENTHESIS_CLOSE, std::regex(R"(\))")},
      {EQU, std::regex(R"(=)")},
      {TIMES, std::regex(R"(\*)")},
      {SLASH, std::regex(R"(/)")},
      {PLUS, std::regex(R"(\+)")},
      {MINUS, std::regex(R"(-)")},

      {AND, std::regex(R"(e|&&)")},
      {OR, std::regex(R"(ou|\|\|)")},

      {NOT, std::regex(R"(!)")},
      {EQUAL, std::regex(R"(==)")},
      {NOT_EQUAL, std::regex(R"(!=)")},
      {GREATER, std::regex(R"(>)")},
      {GREATER_OR_EQUAL, std::regex(R"(>=)")},
      {LOWER, std::regex(R"(<)")},
      {LOWER_OR_EQUAL, std::regex(R"(<=)")},

      {NUM, std::regex(R"(\d+(\.\d+)?)")},
      {TEXT, std::regex(R"(("(\\"|[^"])*"))")},
      {TYPE_REAL, std::regex(R"(numero)")},
      {TYPE_BOOLEANO, std::regex(R"(booleano)")},
      {TYPE_TEXT, std::regex(R"(texto)")},
      {VAR, std::regex(R"(var)")},
      {KEY_INICIO, std::regex(R"(inicio)")},
      {KEY_ESCREVER, std::regex(R"(escrever)")},
      {KEY_LER, std::regex(R"(ler)")},
      {KEY_FIM, std::regex(R"(fim)")},
      {KEY_SE, std::regex(R"(se)")},
      {KEY_SENAO, std::regex(R"(senao)")},
      {KEY_ENTAO, std::regex(R"(entao)")},
      {KEY_VERDADE, std::regex(R"(verdade)")},
      {KEY_FALSO, std::regex(R"(falso)")},
      {KEY_PARA, std::regex(R"(para)")},
      {KEY_REPETIR, std::regex(R"(repetir)")},
      {KEY_ATE, std::regex(R"(ate)")},
      {KEY_ENQUANTO, std::regex(R"(enquanto)")},
      {ID, std::regex(R"(\w+)")},
  };
  /* clang-format on */
  const auto tokens = tokenization::tokenize<SYMBOL>(dictionary_symbol, ERROR,
                                                     sentence_detached);
  auto sentence_symbols = std::vector<SYMBOL>{};
  auto sentence_values = std::vector<std::string>{};

  for (const auto [symbol, value] : tokens) {
    sentence_symbols.push_back(symbol);
    sentence_values.push_back(value);
    // cout << ILC::symbol::symbols_string[symbol];
  }
  return std::tuple(sentence_symbols, sentence_values);
}

int main() {
  std::string input = R"(
    inicio 
      var a numero
      a = 12
      escrever "antes do se\n"
      se a == 10 entao
        escrever "a eh 10\n"
      senao se a == 11 entao 
        escrever "a eh 11\n"
      senao
        escrever "a nao eh 10 nem 11\n"
      fim
      escrever "depois do se\n"
      para i = 1 ate 5+1 repetir
        para j = 1 ate i+1 repetir
          escrever "*"
        fim
        escrever "\n"
      fim
    fim
  )";

  auto [schain, vchain] = get_sentence_elements(input);
  for (const auto &sym : schain) {
    std::cerr << "{" << sym << "} ";
  }
  std::cerr << std::endl;
  for (const auto &sym : vchain) {
    std::cerr << "{" << sym << "} ";
  }
  std::cerr << std::endl;
  chain = std::move(schain);
  str_chain = std::move(vchain);
  chain_size = chain.size();

  serra_jit::context = jit_context_create();
  jit_context_build_start(serra_jit::context);
  jit_type_t params[0];
  jit_type_t signature;

  signature =
      jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 0, 1);
  serra_jit::function = jit_function_create(serra_jit::context, signature);
  bool is_valid = _parse();
  if (not is_valid) {
    std::cerr << "Houve um erro de sintaxe " << std::endl;
  }
  int compiled = jit_compile(serra_jit::function);
  jit_context_build_end(serra_jit::context);
  if (compiled) {
    void *args[0];
    jit_int result;
    jit_function_apply(serra_jit::function, args, &result);
  }
  // std::cerr << (is_valid ? "is valid" : "is not valid") << std::endl;
}
