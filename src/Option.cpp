#include <rl_option.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// rl_option.hpp stress test: lexer + recursive-descent parser + semantic analyzer.
// C++23. Sum-type and scalar/enum dispatch use rl::Option::match() and >>=;
// no switch statements remain in the front end.
// This intentionally stresses template instantiation: three independent sum
// types, recursive ASTs, many match sites, value patterns, generic predicates,
// typed handler adapters, and heterogeneous results wrapped by Option.

consteval std::size_t padding_for_alignment_(std::size_t size_,
                                            std::size_t alignment_) {
  return (alignment_ - (size_ % alignment_)) % alignment_;
}

template <std::size_t Size_> struct PaddingBytes_ {
  unsigned char bytes[Size_]{};
};

template <> struct PaddingBytes_<0> {};

#include <iostream>

template <class T> struct IsExactly {
  template <class U> constexpr bool operator()(const U &) const noexcept {
    return std::same_as<std::remove_cvref_t<U>, T>;
  }
};

template <class T, class F> struct TypedHandler {
  F function;
  using result_type = std::invoke_result_t<F &, T &>;

  result_type operator()() { std::unreachable(); }

  template <class U> result_type operator()(U &&value) {
    if constexpr (std::same_as<std::remove_cvref_t<U>, T>) {
      if constexpr (std::is_void_v<result_type>) {
        std::invoke(function, std::forward<U>(value));
        return;
      } else {
        return std::invoke(function, std::forward<U>(value));
      }
    } else {
      std::unreachable();
    }
  }
};

template <class T, class F> auto on_type(F &&function) {
  return TypedHandler<T, std::decay_t<F>>{std::forward<F>(function)};
}

template <class R> struct WildcardHandler {
  R operator()() const { return R{}; }
  template <class U> R operator()(U &&) const { return R{}; }
};

// Lift a scalar/enum into a one-alternative Option so the library's value
// patterns (rl::Some<T>{value}), callable predicates, and >>= handlers can
// dispatch it too. This is deliberately local to the stress program: the
// library itself remains unchanged.
template <class T, class... Arms> auto match_value(T value, Arms &&...arms) {
  rl::Option<T> subject{rl::Some<T>{std::move(value)}};
  return subject.match(std::forward<Arms>(arms)...);
}

template <> struct WildcardHandler<void> {
  void operator()() const noexcept {}
  template <class U> void operator()(U &&) const noexcept {}
};

// Extract one known alternative. The fallback is intentionally explicit:
// this helper returns a default value on the wrong alternative, not a result
// indicating success. Call only after checking the alternative's tag.
template <class T, class Sum> T extract_alternative(Sum &sum) {
  auto result = sum.match(
      IsExactly<T>{} >>= on_type<T>([](T &value) -> T { return value; }),
      rl::None >>= []() -> T { return T{}; },
      rl::Wild >>= WildcardHandler<T>{});
  return result.unwrap();
}

// -----------------------------------------------------------------------------
// 1. Lexical model: Token is a genuine Option<T...> sum type
// -----------------------------------------------------------------------------

enum class Keyword : std::uint8_t {
  Let,
  Fn,
  Return,
  If,
  Else,
  While,
  Print,
  True,
  False,
  And,
  Or,
  Not,
  Break,
  Continue
};

enum class Symbol : std::uint8_t {
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  Comma,
  Colon,
  Semicolon,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Bang,
  BangEqual,
  Equal,
  EqualEqual,
  Greater,
  GreaterEqual,
  Less,
  LessEqual,
  AndAnd,
  OrOr,
  Arrow
};

struct IdentifierToken {
  std::string_view lexeme;
  int line{1};
  int column{1};
};
struct alignas ( 8 ) NumberToken {
  std::string_view lexeme;
  double value{0.0};
  int line{1};
  int column{1};
  bool integral{true};
  PaddingBytes_<padding_for_alignment_(
      sizeof(std::string_view) + sizeof(double) + 2 * sizeof(int) +
          sizeof(bool),
      8)> _padding{};
};
struct StringToken {
  std::string_view lexeme;
  std::string value;
  int line{1};
  int column{1};
};
struct alignas ( 8 ) KeywordToken {
  std::string_view lexeme;
  int line{1};
  int column{1};
  Keyword keyword{Keyword::Let};
  PaddingBytes_<padding_for_alignment_(
      sizeof(std::string_view) + 2 * sizeof(int) + sizeof(Keyword),
      8)> _padding{};
};
struct alignas ( 8 ) SymbolToken {
  std::string_view lexeme;
  int line{1};
  int column{1};
  Symbol symbol{Symbol::Semicolon};
  PaddingBytes_<padding_for_alignment_(
      sizeof(std::string_view) + 2 * sizeof(int) + sizeof(Symbol),
      8)> _padding{};
};
struct EndToken {
  int line{1};
  int column{1};
};
struct ErrorToken {
  std::string message;
  std::string_view lexeme;
  int line{1};
  int column{1};
};

using Token = rl::Option<IdentifierToken, NumberToken, StringToken,
                          KeywordToken, SymbolToken, EndToken, ErrorToken>;

enum class TokenKind : std::uint8_t {
  Identifier,
  Number,
  String,
  Keyword,
  Symbol,
  End,
  Error,
  None
};

TokenKind token_kind(Token &token) {
  return token
      .match(
          IsExactly<IdentifierToken>{} >>= on_type<IdentifierToken>(
              [](IdentifierToken &) { return TokenKind::Identifier; }),
          IsExactly<NumberToken>{} >>=
          on_type<NumberToken>([](NumberToken &) { return TokenKind::Number; }),
          IsExactly<StringToken>{} >>=
          on_type<StringToken>([](StringToken &) { return TokenKind::String; }),
          IsExactly<KeywordToken>{} >>= on_type<KeywordToken>(
              [](KeywordToken &) { return TokenKind::Keyword; }),
          IsExactly<SymbolToken>{} >>=
          on_type<SymbolToken>([](SymbolToken &) { return TokenKind::Symbol; }),
          IsExactly<EndToken>{} >>=
          on_type<EndToken>([](EndToken &) { return TokenKind::End; }),
          IsExactly<ErrorToken>{} >>=
          on_type<ErrorToken>([](ErrorToken &) { return TokenKind::Error; }),
          rl::None >>= [] { return TokenKind::None; },
          rl::Wild >>= WildcardHandler<TokenKind>{})
      .unwrap();
}

std::string_view token_lexeme(Token &token) {
  return token
      .match(
          IsExactly<IdentifierToken>{} >>=
          on_type<IdentifierToken>([](IdentifierToken &t) { return t.lexeme; }),
          IsExactly<NumberToken>{} >>=
          on_type<NumberToken>([](NumberToken &t) { return t.lexeme; }),
          IsExactly<StringToken>{} >>=
          on_type<StringToken>([](StringToken &t) { return t.lexeme; }),
          IsExactly<KeywordToken>{} >>=
          on_type<KeywordToken>([](KeywordToken &t) { return t.lexeme; }),
          IsExactly<SymbolToken>{} >>=
          on_type<SymbolToken>([](SymbolToken &t) { return t.lexeme; }),
          IsExactly<EndToken>{} >>=
          on_type<EndToken>([](EndToken &) { return std::string_view{"<eof>"}; }),
          IsExactly<ErrorToken>{} >>=
          on_type<ErrorToken>([](ErrorToken &t) { return t.lexeme; }),
          rl::None >>= [] { return std::string_view{"<none>"}; },
          rl::Wild >>= WildcardHandler<std::string_view>{})
      .unwrap();
}

std::pair<int, int> token_position(Token &token) {
  return token
      .match(
          IsExactly<IdentifierToken>{} >>= on_type<IdentifierToken>(
              [](IdentifierToken &t) { return std::pair{t.line, t.column}; }),
          IsExactly<NumberToken>{} >>= on_type<NumberToken>(
              [](NumberToken &t) { return std::pair{t.line, t.column}; }),
          IsExactly<StringToken>{} >>= on_type<StringToken>(
              [](StringToken &t) { return std::pair{t.line, t.column}; }),
          IsExactly<KeywordToken>{} >>= on_type<KeywordToken>(
              [](KeywordToken &t) { return std::pair{t.line, t.column}; }),
          IsExactly<SymbolToken>{} >>= on_type<SymbolToken>(
              [](SymbolToken &t) { return std::pair{t.line, t.column}; }),
          IsExactly<EndToken>{} >>= on_type<EndToken>(
              [](EndToken &t) { return std::pair{t.line, t.column}; }),
          IsExactly<ErrorToken>{} >>= on_type<ErrorToken>(
              [](ErrorToken &t) { return std::pair{t.line, t.column}; }),
          rl::None >>= [] { return std::pair{0, 0}; },
          rl::Wild >>= WildcardHandler<std::pair<int, int>>{})
      .unwrap();
}

Symbol token_symbol(Token &token) {
  return token
      .match(
          IsExactly<SymbolToken>{} >>=
          on_type<SymbolToken>([](SymbolToken &t) { return t.symbol; }),
          rl::None >>= [] { return Symbol::Semicolon; },
          rl::Wild >>= WildcardHandler<Symbol>{})
      .unwrap();
}

Keyword token_keyword(Token &token) {
  return token
      .match(
          IsExactly<KeywordToken>{} >>=
          on_type<KeywordToken>([](KeywordToken &t) { return t.keyword; }),
          rl::None >>= [] { return Keyword::Let; },
          rl::Wild >>= WildcardHandler<Keyword>{})
      .unwrap();
}

// -----------------------------------------------------------------------------
// 2. Lexer
// -----------------------------------------------------------------------------

class alignas ( 8 ) Lexer {
public:
  explicit Lexer(std::string_view source) : source_(source) {}

  std::vector<Token> scan() {
    while (!at_end()) {
      start_ = current_;
      token_line_ = line_;
      token_column_ = column_;
      scan_one();
    }
    tokens_.emplace_back(rl::Some<EndToken>{EndToken{line_, column_}});
    return std::move(tokens_);
  }

private:
  std::string_view source_;
  std::vector<Token> tokens_;
  std::size_t start_{0};
  std::size_t current_{0};
  int line_{1};
  int column_{1};
  int token_line_{1};
  int token_column_{1};

  bool at_end() const noexcept { return current_ >= source_.size(); }
  char peek(std::size_t offset = 0) const noexcept {
    const auto p = current_ + offset;
    return p < source_.size() ? source_[p] : '\0';
  }
  char advance() noexcept {
    const char c = source_[current_++];
    if (c == '\n') {
      ++line_;
      column_ = 1;
    } else {
      ++column_;
    }
    return c;
  }
  bool take(char expected) noexcept {
    if (at_end() || peek() != expected)
      return false;
    (void)advance();
    return true;
  }
  std::string_view slice() const {
    return source_.substr(start_, current_ - start_);
  }

  template <class T> void emit(T value) {
    tokens_.emplace_back(rl::Some<T>{std::move(value)});
  }
  void emit_symbol(Symbol symbol, std::string_view lexeme) {
    emit(SymbolToken{std::move(lexeme), token_line_, token_column_, symbol});
  }
  void error(std::string message) {
    std::cerr << "[lexer " << token_line_ << ':' << token_column_ << "] "
              << message << '\n';
    emit(ErrorToken{std::move(message), slice(), token_line_, token_column_});
  }

  void scan_one() {
    const char c = advance();
    match_value(
        c,
        rl::Some<char>{'('} >>=
        [&](char) { emit_symbol(Symbol::LeftParen, "("); },
        rl::Some<char>{')'} >>=
        [&](char) { emit_symbol(Symbol::RightParen, ")"); },
        rl::Some<char>{'{'} >>=
        [&](char) { emit_symbol(Symbol::LeftBrace, "{"); },
        rl::Some<char>{'}'} >>=
        [&](char) { emit_symbol(Symbol::RightBrace, "}"); },
        rl::Some<char>{','} >>= [&](char) { emit_symbol(Symbol::Comma, ","); },
        rl::Some<char>{':'} >>= [&](char) { emit_symbol(Symbol::Colon, ":"); },
        rl::Some<char>{';'} >>=
        [&](char) { emit_symbol(Symbol::Semicolon, ";"); },
        rl::Some<char>{'+'} >>= [&](char) { emit_symbol(Symbol::Plus, "+"); },
        rl::Some<char>{'*'} >>= [&](char) { emit_symbol(Symbol::Star, "*"); },
        rl::Some<char>{'%'} >>=
        [&](char) { emit_symbol(Symbol::Percent, "%"); },
        rl::Some<char>{'-'} >>=
        [&](char) {
          const bool arrow = take('>');
          emit_symbol(arrow ? Symbol::Arrow : Symbol::Minus, slice());
        },
        rl::Some<char>{'!'} >>=
        [&](char) {
          const bool equal = take('=');
          emit_symbol(equal ? Symbol::BangEqual : Symbol::Bang, slice());
        },
        rl::Some<char>{'='} >>=
        [&](char) {
          const bool equal = take('=');
          emit_symbol(equal ? Symbol::EqualEqual : Symbol::Equal, slice());
        },
        rl::Some<char>{'<'} >>=
        [&](char) {
          const bool equal = take('=');
          emit_symbol(equal ? Symbol::LessEqual : Symbol::Less, slice());
        },
        rl::Some<char>{'>'} >>=
        [&](char) {
          const bool equal = take('=');
          emit_symbol(equal ? Symbol::GreaterEqual : Symbol::Greater, slice());
        },
        rl::Some<char>{'&'} >>=
        [&](char) {
          if (take('&'))
            emit_symbol(Symbol::AndAnd, "&&");
          else
            error("single '&' is not a token; expected '&&'");
        },
        rl::Some<char>{'|'} >>=
        [&](char) {
          if (take('|'))
            emit_symbol(Symbol::OrOr, "||");
          else
            error("single '|' is not a token; expected '||'");
        },
        rl::Some<char>{'/'} >>=
        [&](char) {
          if (take('/')) {
            while (!at_end() && peek() != '\n')
              (void)advance();
          } else if (take('*')) {
            scan_block_comment();
          } else {
            emit_symbol(Symbol::Slash, "/");
          }
        },
        [](char ch) {
          return ch == ' ' || ch == '\r' || ch == '\t' || ch == '\n';
        } >>= [&](char) {},
        rl::Some<char>{'"'} >>= [&](char) { scan_string(); },
        rl::Wild >>=
        [&] {
          if (std::isdigit(static_cast<unsigned char>(c))) {
            scan_number();
            return;
          }
          if (is_alpha(c)) {
            scan_identifier();
            return;
          }
          error(std::format("unexpected character: {}", c));
        });
  }

  static bool is_alpha(char c) noexcept {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
  }
  static bool is_alnum(char c) noexcept {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  }

  void scan_number() {
    while (std::isdigit(static_cast<unsigned char>(peek())))
      (void)advance();
    bool integral = true;
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
      integral = false;
      (void)advance();
      while (std::isdigit(static_cast<unsigned char>(peek())))
        (void)advance();
    }
    if (peek() == 'e' || peek() == 'E') {
      integral = false;
      (void)advance();
      if (peek() == '+' || peek() == '-')
        (void)advance();
      if (!std::isdigit(static_cast<unsigned char>(peek()))) {
        error("malformed exponent in numeric literal");
        return;
      }
      while (std::isdigit(static_cast<unsigned char>(peek())))
        (void)advance();
    }
    const auto text = slice();
    try {
      emit(NumberToken{text, std::stod(std::string{text}), token_line_,
                       token_column_, integral});
    } catch (...) {
      error("numeric literal is out of range");
    }
  }

  void scan_string() {
    std::string value;
    bool terminated = false;
    while (!at_end()) {
      const char c = advance();
      if (c == '"') {
        terminated = true;
        break;
      }
      if (c == '\\') {
        if (at_end())
          break;
        const char escaped = advance();
        match_value(
            escaped,
            rl::Some<char>{'n'} >>= [&](char) { value.push_back('\n'); },
            rl::Some<char>{'r'} >>= [&](char) { value.push_back('\r'); },
            rl::Some<char>{'t'} >>= [&](char) { value.push_back('\t'); },
            rl::Some<char>{'"'} >>= [&](char) { value.push_back('"'); },
            rl::Some<char>{'\\'} >>= [&](char) { value.push_back('\\'); },
            rl::Wild >>=
            [&] {
              error("unsupported string escape");
              value.push_back(escaped);
            });
      } else {
        value.push_back(c);
      }
    }
    if (!terminated) {
      error("unterminated string literal");
      return;
    }
    emit(StringToken{slice(), std::move(value), token_line_, token_column_});
  }

  void scan_block_comment() {
    unsigned depth = 1;
    while (!at_end() && depth != 0) {
      if (peek() == '/' && peek(1) == '*') {
        (void)advance();
        (void)advance();
        ++depth;
      } else if (peek() == '*' && peek(1) == '/') {
        (void)advance();
        (void)advance();
        --depth;
      } else {
        (void)advance();
      }
    }
    if (depth != 0)
      error("unterminated block comment");
  }

  void scan_identifier() {
    while (is_alnum(peek()))
      (void)advance();
    auto text = slice();
    static const std::unordered_map<std::string_view, Keyword> keywords{
        {"let", Keyword::Let},       {"fn", Keyword::Fn},
        {"return", Keyword::Return}, {"if", Keyword::If},
        {"else", Keyword::Else},     {"while", Keyword::While},
        {"print", Keyword::Print},   {"true", Keyword::True},
        {"false", Keyword::False},   {"and", Keyword::And},
        {"or", Keyword::Or},         {"not", Keyword::Not},
        {"break", Keyword::Break},   {"continue", Keyword::Continue}};
    if (const auto it = keywords.find(text); it != keywords.end()) {
      emit(KeywordToken{std::move(text), token_line_, token_column_,
                        it->second});
    } else {
      emit(IdentifierToken{std::move(text), token_line_, token_column_});
    }
  }
};

// -----------------------------------------------------------------------------
// 3. AST: three recursively connected Option<T...> sum types overall
// -----------------------------------------------------------------------------

enum class StaticType : std::uint8_t {
  Error,
  Int,
  Float,
  Bool,
  String,
  Void,
  Function
};
enum class UnaryOp : std::uint8_t { Negate, Positive, Not };
enum class BinaryOp : std::uint8_t {
  Add,
  Subtract,
  Multiply,
  Divide,
  Remainder,
  Equal,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  LogicalAnd,
  LogicalOr
};

struct Expr;
struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct IntExpr {
  std::int64_t value{0};
};
struct FloatExpr {
  double value{0.0};
};
struct BoolExpr {
  bool value{false};
};
struct StringExpr {
  std::string value;
};
struct NameExpr {
  std::string_view name;
};
struct alignas ( 8 ) UnaryExpr {
  ExprPtr operand;
  UnaryOp op{UnaryOp::Positive};
  PaddingBytes_<padding_for_alignment_(sizeof(ExprPtr) + sizeof(UnaryOp),
                                       alignof(ExprPtr))> _padding{};
};
struct alignas ( 8 ) BinaryExpr {
  ExprPtr left;
  BinaryOp op{BinaryOp::Add};
  PaddingBytes_<padding_for_alignment_(sizeof(ExprPtr) + sizeof(BinaryOp),
                                       alignof(ExprPtr))> _padding{};
  ExprPtr right;
};
struct AssignExpr {
  std::string_view name;
  ExprPtr value;
};
struct CallExpr {
  ExprPtr callee;
  std::vector<ExprPtr> arguments;
};

struct Expr final
    : rl::Option<IntExpr, FloatExpr, BoolExpr, StringExpr, NameExpr, UnaryExpr,
                  BinaryExpr, AssignExpr, CallExpr> {
  using Base = rl::Option<IntExpr, FloatExpr, BoolExpr, StringExpr, NameExpr,
                           UnaryExpr, BinaryExpr, AssignExpr, CallExpr>;
  using Base::Base;
};

template <class T> ExprPtr make_expr(T node) {
  return std::make_shared<Expr>(rl::Some<T>{std::move(node)});
}

struct alignas ( 8 ) Parameter {
  std::string_view name;
  StaticType type{StaticType::Error};
  PaddingBytes_<padding_for_alignment_(
      sizeof(std::string_view) + sizeof(StaticType), 8)> _padding{};
};
struct alignas ( 8 ) LetStmt {
  std::string_view name;
  std::optional<StaticType> annotation;
  PaddingBytes_<padding_for_alignment_(
      sizeof(std::string_view) + sizeof(std::optional<StaticType>),
      alignof(ExprPtr))> _padding{};
  ExprPtr initializer;
};
struct PrintStmt {
  ExprPtr expression;
};
struct ExprStmt {
  ExprPtr expression;
};
struct BlockStmt {
  std::vector<StmtPtr> statements;
};
struct IfStmt {
  ExprPtr condition;
  StmtPtr then_branch;
  StmtPtr else_branch;
};
struct WhileStmt {
  ExprPtr condition;
  StmtPtr body;
};
struct alignas ( 8 ) FunctionStmt {
  std::string_view name;
  std::vector<Parameter> parameters;
  StaticType return_type{StaticType::Void};
  PaddingBytes_<padding_for_alignment_(
      sizeof(std::string_view) + sizeof(std::vector<Parameter>) +
          sizeof(StaticType),
      alignof(std::vector<StmtPtr>))> _padding{};
  std::vector<StmtPtr> body;
};
struct alignas ( 8 ) ReturnStmt {
  ExprPtr expression;
  bool has_value{true};
  PaddingBytes_<padding_for_alignment_(sizeof(ExprPtr) + sizeof(bool),
                                       alignof(ExprPtr))> _padding{};
};
struct BreakStmt {};
struct ContinueStmt {};

struct Stmt final
    : rl::Option<LetStmt, PrintStmt, ExprStmt, BlockStmt, IfStmt, WhileStmt,
                  FunctionStmt, ReturnStmt, BreakStmt, ContinueStmt> {
  using Base =
      rl::Option<LetStmt, PrintStmt, ExprStmt, BlockStmt, IfStmt, WhileStmt,
                  FunctionStmt, ReturnStmt, BreakStmt, ContinueStmt>;
  using Base::Base;
};

template <class T> StmtPtr make_stmt(T node) {
  return std::make_shared<Stmt>(rl::Some<T>{std::move(node)});
}

// -----------------------------------------------------------------------------
// 4. Parser
// -----------------------------------------------------------------------------

class alignas ( 8 ) ParseFailure final : public std::exception {
public:
  const char *what() const noexcept override { return "parse failure"; }
};

class alignas ( 8 ) Parser {
public:
  explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

  std::vector<StmtPtr> parse() {
    std::vector<StmtPtr> program;
    while (!at_end()) {
      try {
        if (auto statement = declaration())
          program.emplace_back(std::move(statement));
      } catch (const ParseFailure &) {
        synchronize();
      }
    }
    return program;
  }

  const std::vector<std::string> &diagnostics() const noexcept {
    return diagnostics_;
  }

private:
  std::vector<Token> tokens_;
  std::size_t current_{0};
  std::vector<std::string> diagnostics_;

  Token &token() { return tokens_[std::min(current_, tokens_.size() - 1)]; }
  Token &previous() { return tokens_[current_ == 0 ? 0 : current_ - 1]; }
  TokenKind kind() { return token_kind(token()); }
  bool at_end() { return kind() == TokenKind::End; }

  bool check_symbol(Symbol symbol) {
    return kind() == TokenKind::Symbol && token_symbol(token()) == symbol;
  }
  bool check_keyword(Keyword keyword) {
    return kind() == TokenKind::Keyword && token_keyword(token()) == keyword;
  }
  bool match_symbol(Symbol symbol) {
    if (!check_symbol(symbol))
      return false;
    ++current_;
    return true;
  }
  bool match_keyword(Keyword keyword) {
    if (!check_keyword(keyword))
      return false;
    ++current_;
    return true;
  }

  [[noreturn]] void fail(std::string message) {
    auto [line, column] = token_position(token());
    std::string diagnostic = "[parser ";
    diagnostic += std::to_string(line);
    diagnostic += ":";
    diagnostic += std::to_string(column);
    diagnostic += "] ";
    diagnostic += message;
    diagnostic += " near '";
    diagnostic.append(token_lexeme(token()));
    diagnostic += "'";
    diagnostics_.emplace_back(std::move(diagnostic));
    throw ParseFailure{};
  }

  void expect_symbol(Symbol symbol, std::string_view message) {
    if (!match_symbol(symbol))
      fail(std::string(message));
  }
  std::string_view expect_identifier(std::string_view message) {
    if (kind() != TokenKind::Identifier)
      fail(std::string(message));
    auto result = extract_alternative<IdentifierToken>(token()).lexeme;
    ++current_;
    return result;
  }

  void synchronize() {
    if (!at_end())
      ++current_;
    while (!at_end()) {
      if (kind() == TokenKind::Symbol &&
          token_symbol(token()) == Symbol::Semicolon) {
        ++current_;
        return;
      }
      if (check_keyword(Keyword::Let) || check_keyword(Keyword::Fn) ||
          check_keyword(Keyword::If) || check_keyword(Keyword::While) ||
          check_keyword(Keyword::Print) || check_keyword(Keyword::Return))
        return;
      ++current_;
    }
  }

  std::optional<StaticType> parse_type_name() {
    if (kind() != TokenKind::Identifier)
      fail("expected a type name");
    const auto name = extract_alternative<IdentifierToken>(token()).lexeme;
    ++current_;
    if (name == "int")
      return StaticType::Int;
    if (name == "float")
      return StaticType::Float;
    if (name == "bool")
      return StaticType::Bool;
    if (name == "string")
      return StaticType::String;
    if (name == "void")
      return StaticType::Void;
    fail(std::format("unknown type: {}'", name));
  }

  StmtPtr declaration() {
    if (match_keyword(Keyword::Let))
      return variable_declaration();
    if (match_keyword(Keyword::Fn))
      return function_declaration();
    return statement();
  }

  StmtPtr variable_declaration() {
    const auto name = expect_identifier("expected variable name after 'let'");
    std::optional<StaticType> annotation;
    if (match_symbol(Symbol::Colon))
      annotation = parse_type_name();
    expect_symbol(Symbol::Equal, "expected '=' in variable declaration");
    auto initializer = expression();
    expect_symbol(Symbol::Semicolon, "expected ';' after variable declaration");
    return make_stmt(LetStmt{name, annotation, {}, std::move(initializer)});
  }

  StmtPtr function_declaration() {
    const auto name = expect_identifier("expected function name");
    expect_symbol(Symbol::LeftParen, "expected '(' after function name");
    std::vector<Parameter> parameters;
    if (!check_symbol(Symbol::RightParen)) {
      do {
        auto parameter_name = expect_identifier("expected parameter name");
        expect_symbol(Symbol::Colon, "expected ':' after parameter name");
        auto parameter_type = parse_type_name();
        if (!parameter_type)
          fail("expected parameter type");
        parameters.emplace_back(
            Parameter{std::move(parameter_name), *parameter_type});
      } while (match_symbol(Symbol::Comma));
    }
    expect_symbol(Symbol::RightParen, "expected ')' after parameters");
    StaticType return_type = StaticType::Void;
    if (match_symbol(Symbol::Arrow)) {
      auto parsed = parse_type_name();
      if (!parsed)
        fail("expected return type");
      return_type = *parsed;
    }
    expect_symbol(Symbol::LeftBrace, "expected '{' before function body");
    auto body = block_contents();
    return make_stmt(FunctionStmt{name, std::move(parameters), return_type, {},
                                  std::move(body)});
  }

  StmtPtr statement() {
    if (match_keyword(Keyword::Print))
      return print_statement();
    if (match_keyword(Keyword::Return))
      return return_statement();
    if (match_keyword(Keyword::If))
      return if_statement();
    if (match_keyword(Keyword::While))
      return while_statement();
    if (match_keyword(Keyword::Break)) {
      expect_symbol(Symbol::Semicolon, "expected ';' after break");
      return make_stmt(BreakStmt{});
    }
    if (match_keyword(Keyword::Continue)) {
      expect_symbol(Symbol::Semicolon, "expected ';' after continue");
      return make_stmt(ContinueStmt{});
    }
    if (match_symbol(Symbol::LeftBrace))
      return make_stmt(BlockStmt{block_contents()});
    auto expr = expression();
    expect_symbol(Symbol::Semicolon, "expected ';' after expression");
    return make_stmt(ExprStmt{std::move(expr)});
  }

  StmtPtr print_statement() {
    auto value = expression();
    expect_symbol(Symbol::Semicolon, "expected ';' after print expression");
    return make_stmt(PrintStmt{std::move(value)});
  }

  StmtPtr return_statement() {
    ExprPtr value;
    if (!check_symbol(Symbol::Semicolon))
      value = expression();
    expect_symbol(Symbol::Semicolon, "expected ';' after return");
    const bool has_value = static_cast<bool>(value);
    if (!value)
      value = make_expr(
          BoolExpr{false}); // storage placeholder; has_value carries semantics
    return make_stmt(ReturnStmt{std::move(value), has_value});
  }

  StmtPtr if_statement() {
    expect_symbol(Symbol::LeftParen, "expected '(' after if");
    auto condition = expression();
    expect_symbol(Symbol::RightParen, "expected ')' after if condition");
    auto then_branch = statement();
    StmtPtr else_branch;
    if (match_keyword(Keyword::Else))
      else_branch = statement();
    return make_stmt(IfStmt{std::move(condition), std::move(then_branch),
                            std::move(else_branch)});
  }

  StmtPtr while_statement() {
    expect_symbol(Symbol::LeftParen, "expected '(' after while");
    auto condition = expression();
    expect_symbol(Symbol::RightParen, "expected ')' after while condition");
    auto body = statement();
    return make_stmt(WhileStmt{std::move(condition), std::move(body)});
  }

  std::vector<StmtPtr> block_contents() {
    std::vector<StmtPtr> statements;
    while (!check_symbol(Symbol::RightBrace) && !at_end()) {
      if (auto statement = declaration())
        statements.emplace_back(std::move(statement));
    }
    expect_symbol(Symbol::RightBrace, "expected '}' after block");
    return statements;
  }

  // Expression grammar, from lowest to highest precedence:
  // assignment -> or -> and -> equality -> comparison -> term -> factor
  //             -> unary -> call -> primary.
  ExprPtr expression() { return assignment(); }

  ExprPtr assignment() {
    auto left = logical_or();
    if (!match_symbol(Symbol::Equal))
      return left;
    auto right = assignment();
    auto name = left->match(
                        IsExactly<NameExpr>{} >>=
                        on_type<NameExpr>([](NameExpr &n) { return n.name; }),
                        rl::None >>= [] { return std::string_view{}; },
                        rl::Wild >>= WildcardHandler<std::string_view>{})
                    .unwrap();
    if (name.empty())
      fail("assignment target must be an identifier");
    return make_expr(AssignExpr{std::move(name), std::move(right)});
  }

  ExprPtr logical_or() {
    auto expr = logical_and();
    while (match_symbol(Symbol::OrOr) || match_keyword(Keyword::Or)) {
      auto right = logical_and();
      expr = make_expr(
          BinaryExpr{std::move(expr), BinaryOp::LogicalOr, {},
                     std::move(right)});
    }
    return expr;
  }
  ExprPtr logical_and() {
    auto expr = equality();
    while (match_symbol(Symbol::AndAnd) || match_keyword(Keyword::And)) {
      auto right = equality();
      expr = make_expr(
          BinaryExpr{std::move(expr), BinaryOp::LogicalAnd, {},
                     std::move(right)});
    }
    return expr;
  }
  ExprPtr equality() {
    auto expr = comparison();
    for (;;) {
      BinaryOp op;
      if (match_symbol(Symbol::EqualEqual))
        op = BinaryOp::Equal;
      else if (match_symbol(Symbol::BangEqual))
        op = BinaryOp::NotEqual;
      else
        break;
      auto right = comparison();
      expr =
          make_expr(BinaryExpr{std::move(expr), op, {}, std::move(right)});
    }
    return expr;
  }
  ExprPtr comparison() {
    auto expr = term();
    for (;;) {
      BinaryOp op;
      if (match_symbol(Symbol::Less))
        op = BinaryOp::Less;
      else if (match_symbol(Symbol::LessEqual))
        op = BinaryOp::LessEqual;
      else if (match_symbol(Symbol::Greater))
        op = BinaryOp::Greater;
      else if (match_symbol(Symbol::GreaterEqual))
        op = BinaryOp::GreaterEqual;
      else
        break;
      auto right = term();
      expr =
          make_expr(BinaryExpr{std::move(expr), op, {}, std::move(right)});
    }
    return expr;
  }
  ExprPtr term() {
    auto expr = factor();
    for (;;) {
      BinaryOp op;
      if (match_symbol(Symbol::Plus))
        op = BinaryOp::Add;
      else if (match_symbol(Symbol::Minus))
        op = BinaryOp::Subtract;
      else
        break;
      auto right = factor();
      expr =
          make_expr(BinaryExpr{std::move(expr), op, {}, std::move(right)});
    }
    return expr;
  }
  ExprPtr factor() {
    auto expr = unary();
    for (;;) {
      BinaryOp op;
      if (match_symbol(Symbol::Star))
        op = BinaryOp::Multiply;
      else if (match_symbol(Symbol::Slash))
        op = BinaryOp::Divide;
      else if (match_symbol(Symbol::Percent))
        op = BinaryOp::Remainder;
      else
        break;
      auto right = unary();
      expr =
          make_expr(BinaryExpr{std::move(expr), op, {}, std::move(right)});
    }
    return expr;
  }
  ExprPtr unary() {
    if (match_symbol(Symbol::Bang) || match_keyword(Keyword::Not)) {
      return make_expr(UnaryExpr{unary(), UnaryOp::Not});
    }
    if (match_symbol(Symbol::Minus))
      return make_expr(UnaryExpr{unary(), UnaryOp::Negate});
    if (match_symbol(Symbol::Plus))
      return make_expr(UnaryExpr{unary(), UnaryOp::Positive});
    return call();
  }
  ExprPtr call() {
    auto expr = primary();
    for (;;) {
      if (!match_symbol(Symbol::LeftParen))
        break;
      std::vector<ExprPtr> arguments;
      if (!check_symbol(Symbol::RightParen)) {
        do {
          arguments.emplace_back(expression());
        } while (match_symbol(Symbol::Comma));
      }
      expect_symbol(Symbol::RightParen, "expected ')' after call arguments");
      expr = make_expr(CallExpr{std::move(expr), std::move(arguments)});
    }
    return expr;
  }
  ExprPtr primary() {
    if (kind() == TokenKind::Number) {
      auto number = extract_alternative<NumberToken>(token());
      ++current_;
      if (number.integral) {
        return make_expr(IntExpr{static_cast<std::int64_t>(number.value)});
      }
      return make_expr(FloatExpr{number.value});
    }
    if (kind() == TokenKind::String) {
      auto string = extract_alternative<StringToken>(token());
      ++current_;
      return make_expr(StringExpr{std::move(string.value)});
    }
    if (check_keyword(Keyword::True) || check_keyword(Keyword::False)) {
      const bool value = check_keyword(Keyword::True);
      ++current_;
      return make_expr(BoolExpr{value});
    }
    if (kind() == TokenKind::Identifier) {
      auto name = extract_alternative<IdentifierToken>(token()).lexeme;
      ++current_;
      return make_expr(NameExpr{std::move(name)});
    }
    if (match_symbol(Symbol::LeftParen)) {
      auto expr = expression();
      expect_symbol(Symbol::RightParen,
                    "expected ')' after grouped expression");
      return expr;
    }
    fail("expected expression");
  }
};

// -----------------------------------------------------------------------------
// 5. Semantic analysis
// -----------------------------------------------------------------------------

std::string_view type_name(StaticType type) {
  return match_value(
             type,
             rl::Some<StaticType>{StaticType::Error} >>=
             [](StaticType) { return std::string_view{"<error>"}; },
             rl::Some<StaticType>{StaticType::Int} >>=
             [](StaticType) { return std::string_view{"int"}; },
             rl::Some<StaticType>{StaticType::Float} >>=
             [](StaticType) { return std::string_view{"float"}; },
             rl::Some<StaticType>{StaticType::Bool} >>=
             [](StaticType) { return std::string_view{"bool"}; },
             rl::Some<StaticType>{StaticType::String} >>=
             [](StaticType) { return std::string_view{"string"}; },
             rl::Some<StaticType>{StaticType::Void} >>=
             [](StaticType) { return std::string_view{"void"}; },
             rl::Some<StaticType>{StaticType::Function} >>=
             [](StaticType) { return std::string_view{"function"}; },
             rl::Wild >>= [] { return std::string_view{"<invalid-type>"}; })
      .unwrap();
}

class alignas ( 8 ) SemanticAnalyzer {
public:
  void analyze(const std::vector<StmtPtr> &program) {
    // Pass 1: collect function signatures, so recursion and forward calls work.
    for (const auto &statement : program)
      predeclare_function(statement);
    // Pass 2: check declarations, expressions, scopes, and control flow.
    for (const auto &statement : program)
      analyze_statement(statement);
  }

  const std::vector<std::string> &diagnostics() const noexcept {
    return diagnostics_;
  }

private:
  struct alignas ( 8 ) FunctionInfo {
    std::vector<StaticType> parameters;
    StaticType result{StaticType::Void};
    PaddingBytes_<padding_for_alignment_(
        sizeof(std::vector<StaticType>) + sizeof(StaticType), 8)> _padding{};
  };
  std::vector<std::unordered_map<std::string_view, StaticType>> scopes_{{}};
  std::unordered_map<std::string_view, FunctionInfo> functions_;
  std::vector<std::string> diagnostics_;
  StaticType expected_return_{StaticType::Void};
  bool in_function_{false};
  PaddingBytes_<padding_for_alignment_(sizeof(StaticType) + sizeof(bool),
                                       alignof(unsigned))> _loop_depth_pad{};
  unsigned loop_depth_{0};

  void report(std::string message) {
    diagnostics_.emplace_back("[sema] " + std::move(message));
  }
  void push_scope() { scopes_.emplace_back(); }
  void pop_scope() {
    if (scopes_.size() > 1)
      scopes_.pop_back();
  }

  bool declare(std::string_view name, StaticType type) {
    auto &scope = scopes_.back();
    if (scope.contains(name)) {
      report(std::format("duplicate declaration of '{}'", name));
      return false;
    }
    scope.emplace(std::move(name), type);
    return true;
  }

  std::optional<StaticType> lookup(std::string_view name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (const auto found = it->find(std::string_view(name)); found != it->end())
        return found->second;
    }
    if (functions_.contains(std::string_view(name)))
      return StaticType::Function;
    return std::nullopt;
  }

  bool compatible(StaticType expected, StaticType actual) const {
    if (expected == StaticType::Error || actual == StaticType::Error)
      return true;
    return expected == actual ||
           (expected == StaticType::Float && actual == StaticType::Int);
  }
  bool numeric(StaticType type) const {
    return type == StaticType::Int || type == StaticType::Float;
  }
  StaticType promote(StaticType a, StaticType b) const {
    if (!numeric(a) || !numeric(b))
      return StaticType::Error;
    return (a == StaticType::Float || b == StaticType::Float)
               ? StaticType::Float
               : StaticType::Int;
  }

  void predeclare_function(const StmtPtr &statement) {
    statement->match(
        IsExactly<FunctionStmt>{} >>=
        on_type<FunctionStmt>([&](FunctionStmt &function) {
          FunctionInfo info;
          for (const auto &parameter : function.parameters)
            info.parameters.emplace_back(parameter.type);
          info.result = function.return_type;
          if (functions_.contains(function.name))
            report(std::format("duplicate function '{}'", function.name));
          else
            functions_.emplace(function.name, std::move(info));
        }),
        rl::None >>= [] {}, rl::Wild >>= WildcardHandler<void>{});
  }

  StaticType analyze_expression(const ExprPtr &expression) {
    if (!expression)
      return StaticType::Error;
    return expression
        ->match(
            IsExactly<IntExpr>{} >>=
            on_type<IntExpr>([](IntExpr &) { return StaticType::Int; }),
            IsExactly<FloatExpr>{} >>=
            on_type<FloatExpr>([](FloatExpr &) { return StaticType::Float; }),
            IsExactly<BoolExpr>{} >>=
            on_type<BoolExpr>([](BoolExpr &) { return StaticType::Bool; }),
            IsExactly<StringExpr>{} >>= on_type<StringExpr>(
                [](StringExpr &) { return StaticType::String; }),
            IsExactly<NameExpr>{} >>= on_type<NameExpr>([&](NameExpr &name) {
              if (auto found = lookup(name.name))
                return *found;
              report(std::format("use of undeclared identifier '{}'", name.name));
              return StaticType::Error;
            }),
            IsExactly<UnaryExpr>{} >>=
            on_type<UnaryExpr>([&](UnaryExpr &unary) {
              const auto operand = analyze_expression(unary.operand);
              return match_value(
                         unary.op,
                         rl::Some<UnaryOp>{UnaryOp::Not} >>=
                         [&](UnaryOp) {
                           if (operand != StaticType::Bool &&
                               operand != StaticType::Error)
                             report(std::format("logical negation requires bool, got {}",
                                                type_name(operand)));
                           return operand == StaticType::Error
                                      ? StaticType::Error
                                      : StaticType::Bool;
                         },
                         [](UnaryOp op) {
                           return op == UnaryOp::Negate ||
                                  op == UnaryOp::Positive;
                         } >>=
                         [&](UnaryOp) {
                           if (!numeric(operand) &&
                               operand != StaticType::Error)
                             report("numeric unary operator requires int or "
                                    "float");
                           return numeric(operand) ? operand
                                                   : StaticType::Error;
                         },
                         rl::Wild >>= [] { return StaticType::Error; })
                  .unwrap();
            }),
            IsExactly<BinaryExpr>{} >>=
            on_type<BinaryExpr>([&](BinaryExpr &binary) {
              const auto left = analyze_expression(binary.left);
              const auto right = analyze_expression(binary.right);
              return match_value(
                         binary.op,
                         rl::Some<BinaryOp>{BinaryOp::Add} >>=
                         [&](BinaryOp) {
                           if (left == StaticType::String &&
                               right == StaticType::String)
                             return StaticType::String;
                           if (numeric(left) && numeric(right))
                             return promote(left, right);
                           report("operator '+' requires two numbers or two "
                                  "strings");
                           return StaticType::Error;
                         },
                         [](BinaryOp op) {
                           return op == BinaryOp::Subtract ||
                                  op == BinaryOp::Multiply ||
                                  op == BinaryOp::Divide ||
                                  op == BinaryOp::Remainder;
                         } >>=
                         [&](BinaryOp) {
                           if (numeric(left) && numeric(right))
                             return promote(left, right);
                           report(
                               "arithmetic operator requires numeric operands");
                           return StaticType::Error;
                         },
                         [](BinaryOp op) {
                           return op == BinaryOp::Less ||
                                  op == BinaryOp::LessEqual ||
                                  op == BinaryOp::Greater ||
                                  op == BinaryOp::GreaterEqual;
                         } >>=
                         [&](BinaryOp) {
                           if ((numeric(left) && numeric(right)) ||
                               (left == StaticType::String &&
                                right == StaticType::String))
                             return StaticType::Bool;
                           report("ordered comparison requires compatible "
                                  "numeric or string operands");
                           return StaticType::Error;
                         },
                         [](BinaryOp op) {
                           return op == BinaryOp::Equal ||
                                  op == BinaryOp::NotEqual;
                         } >>=
                         [&](BinaryOp) {
                           if (compatible(left, right))
                             return StaticType::Bool;
                           report("equality comparison between incompatible "
                                  "types");
                           return StaticType::Error;
                         },
                         [](BinaryOp op) {
                           return op == BinaryOp::LogicalAnd ||
                                  op == BinaryOp::LogicalOr;
                         } >>=
                         [&](BinaryOp) {
                           if ((left == StaticType::Bool ||
                                left == StaticType::Error) &&
                               (right == StaticType::Bool ||
                                right == StaticType::Error))
                             return StaticType::Bool;
                           report("logical operators require bool operands");
                           return StaticType::Error;
                         },
                         rl::Wild >>= [] { return StaticType::Error; })
                  .unwrap();
            }),
            IsExactly<AssignExpr>{} >>=
            on_type<AssignExpr>([&](AssignExpr &assignment) {
              const auto value_type = analyze_expression(assignment.value);
              auto target_type = lookup(assignment.name);
              if (!target_type) {
                report(std::format("assignment to undeclared identifier '{}'",
                                   assignment.name));
                return StaticType::Error;
              }
              if (!compatible(*target_type, value_type))
                report(std::format("cannot assign {} to {} variable '{}'",
                                    type_name(value_type), type_name(*target_type),
                                    assignment.name));
              return *target_type;
            }),
            IsExactly<CallExpr>{} >>= on_type<CallExpr>([&](CallExpr &call) {
              const auto callee_name =
                  call.callee
                      ->match(
                          IsExactly<NameExpr>{} >>= on_type<NameExpr>(
                              [](NameExpr &name) { return name.name; }),
                          rl::None >>= [] { return std::string_view{}; },
                          rl::Wild >>= WildcardHandler<std::string_view>{})
                      .unwrap();
              if (callee_name.empty()) {
                (void)analyze_expression(call.callee);
                for (const auto &argument : call.arguments)
                  (void)analyze_expression(argument);
                report(
                    "only named functions are callable in this test language");
                return StaticType::Error;
              }
              const auto function = functions_.find(callee_name);
              std::vector<StaticType> actual;
              for (const auto &argument : call.arguments)
                actual.emplace_back(analyze_expression(argument));
              if (function == functions_.end()) {
                report(std::format("call to unknown function '{}'", callee_name));
                return StaticType::Error;
              }
              if (actual.size() != function->second.parameters.size()) {
                report(std::format("function '{}' expects {} arguments, got {}",
                                   callee_name,
                                   function->second.parameters.size(),
                                   actual.size()));
              }
              const auto count =
                  std::min(actual.size(), function->second.parameters.size());
              for (std::size_t i = 0; i < count; ++i) {
                if (!compatible(function->second.parameters[i], actual[i]))
                  report(std::format(
                      "argument {} of '{}' has type {}, expected {}", i + 1,
                      callee_name, type_name(actual[i]),
                      type_name(function->second.parameters[i])));
              }
              return function->second.result;
            }),
            rl::None >>= [] { return StaticType::Error; },
            rl::Wild >>= WildcardHandler<StaticType>{})
        .unwrap();
  }

  void analyze_statement(const StmtPtr &statement) {
    if (!statement)
      return;
    statement->match(
        IsExactly<LetStmt>{} >>= on_type<LetStmt>([&](LetStmt &declaration) {
          const auto actual = analyze_expression(declaration.initializer);
          const auto target = declaration.annotation.value_or(actual);
          if (!compatible(target, actual))
            report(std::format("initializer of '{}' has type {}, expected {}",
                               declaration.name, type_name(actual),
                               type_name(target)));
          declare(declaration.name, target);
        }),
        IsExactly<PrintStmt>{} >>= on_type<PrintStmt>([&](PrintStmt &print) {
          (void)analyze_expression(print.expression);
        }),
        IsExactly<ExprStmt>{} >>= on_type<ExprStmt>([&](ExprStmt &expression) {
          (void)analyze_expression(expression.expression);
        }),
        IsExactly<BlockStmt>{} >>= on_type<BlockStmt>([&](BlockStmt &block) {
          push_scope();
          for (const auto &child : block.statements)
            analyze_statement(child);
          pop_scope();
        }),
        IsExactly<IfStmt>{} >>= on_type<IfStmt>([&](IfStmt &branch) {
          const auto condition = analyze_expression(branch.condition);
          if (condition != StaticType::Bool && condition != StaticType::Error)
            report("if condition must be bool");
          analyze_statement(branch.then_branch);
          if (branch.else_branch)
            analyze_statement(branch.else_branch);
        }),
        IsExactly<WhileStmt>{} >>= on_type<WhileStmt>([&](WhileStmt &loop) {
          const auto condition = analyze_expression(loop.condition);
          if (condition != StaticType::Bool && condition != StaticType::Error)
            report("while condition must be bool");
          ++loop_depth_;
          analyze_statement(loop.body);
          --loop_depth_;
        }),
        IsExactly<FunctionStmt>{} >>=
        on_type<FunctionStmt>([&](FunctionStmt &function) {
          const bool old_in_function = in_function_;
          const auto old_return = expected_return_;
          in_function_ = true;
          expected_return_ = function.return_type;
          push_scope();
          for (const auto &parameter : function.parameters)
            declare(parameter.name, parameter.type);
          for (const auto &child : function.body)
            analyze_statement(child);
          pop_scope();
          expected_return_ = old_return;
          in_function_ = old_in_function;
        }),
        IsExactly<ReturnStmt>{} >>=
        on_type<ReturnStmt>([&](ReturnStmt &result) {
          if (!in_function_) {
            report("return outside function");
            (void)analyze_expression(result.expression);
            return;
          }
          const auto actual = analyze_expression(result.expression);
          if (expected_return_ == StaticType::Void) {
            if (result.has_value)
              report("void function cannot return a value");
          } else if (!result.has_value) {
            report("non-void function must return a value");
          } else if (!compatible(expected_return_, actual)) {
            report(std::format("return type mismatch: expected {}, got {}",
                               type_name(expected_return_), type_name(actual)));
          }
        }),
        IsExactly<BreakStmt>{} >>= on_type<BreakStmt>([&](BreakStmt &) {
          if (loop_depth_ == 0)
            report("break outside loop");
        }),
        IsExactly<ContinueStmt>{} >>=
        on_type<ContinueStmt>([&](ContinueStmt &) {
          if (loop_depth_ == 0)
            report("continue outside loop");
        }),
        rl::None >>= [] {}, rl::Wild >>= WildcardHandler<void>{});
  }
};

// -----------------------------------------------------------------------------
// 6. Stress program: recursive calls, precedence, nested scopes, and
// diagnostics
// -----------------------------------------------------------------------------

constexpr std::string_view stress_program = R"SRC(
/* Nested comments are supported: /* lexer depth test */ still inside. */

fn fib(n: int) -> int {
    if (n <= 1) {
        return n;
    } else {
        return fib(n - 1) + fib(n - 2);
    }
}

fn classify(flag: bool, amount: float) -> string {
    if (flag && !(amount < 0.0 || amount == 0.0)) {
        return "positive";
    } else {
        return "not-positive";
    }
}

fn combine(a: string, b: string, c: string) -> string {
    return a + ":" + b + ":" + c;
}

let limit: int = 9;
let index: int = 0;
let banner: string = combine("Option(4)", "lexer", "parser + sema");
print banner;

while (index < limit) {
    let current: int = fib(index);
    let label: string = classify(current > 2 && current != 5, current * 1.25);
    print combine(label, "fib", "value");
    if (current % 2 == 0 || current == 1) {
        print "even-ish or first";
    } else {
        print "odd";
    }
    index = index + 1;
}

{
    let shadow: int = 100;
    let shadowed: float = shadow + 0.5;
    print shadowed;
}

// Intentional semantic error to exercise error paths without breaking parsing.
let deliberately_bad: bool = true;
print missing_symbol;
)SRC";

// -----------------------------------------------------------------------------
// 7. Driver and a small run-time dispatch report
// -----------------------------------------------------------------------------

int main() {
  std::cout << "Option(4).hpp compiler stress test\n";
  std::cout << "C++ standard: " << __cplusplus << "\n";

  Lexer lexer{stress_program};
  auto tokens = lexer.scan();
  std::cout << "tokens: " << tokens.size() << '\n';

  // A dispatch-only pass forces every token alternative through .match().
  std::size_t token_classes[8]{};
  for (auto &token : tokens) {
    const auto tag = token_kind(token);
    ++token_classes[static_cast<std::size_t>(tag)];
    (void)token_lexeme(token);
    (void)token_position(token);
  }
  std::cout << "token alternatives: identifier=" << token_classes[0]
            << " number=" << token_classes[1] << " string=" << token_classes[2]
            << " keyword=" << token_classes[3] << " symbol=" << token_classes[4]
            << " eof=" << token_classes[5] << " error=" << token_classes[6]
            << '\n';

  Parser parser{std::move(tokens)};
  auto program = parser.parse();
  std::cout << "top-level statements: " << program.size() << '\n';
  for (const auto &diagnostic : parser.diagnostics())
    std::cout << diagnostic << '\n';

  SemanticAnalyzer sema;
  sema.analyze(program);
  std::cout << "semantic diagnostics: " << sema.diagnostics().size() << '\n';
  for (const auto &diagnostic : sema.diagnostics())
    std::cout << diagnostic << '\n';

  if (token_classes[6] != 0)
    return 1; // lexer should be clean
  if (!parser.diagnostics().empty())
    return 2; // sample should parse cleanly
  if (sema.diagnostics().size() != 1)
    return 3; // exactly one intentional semantic error
  return 0;
}
