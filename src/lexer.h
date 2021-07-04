#ifndef COLUMBO_LEXER_H
#define COLUMBO_LEXER_H

#include <cassert>
#include <iostream>
#include <string>

enum class TKind { Invalid, NewLine, EndOfFile, String, Number, HexNumber };

enum class SkipNewLine { True, False };

struct Loc {
  Loc() = default;
  Loc(int l, int c) : line(l), col(c) {}

  friend std::ostream &operator<<(std::ostream &stream, const Loc &loc) {
    stream << loc.line << "," << loc.col;
    return stream;
  }

private:
  int line = 1;
  int col = 1;
};

struct Tok {
  Tok() = default;

  friend struct Lexer;

  // Warning: copies data. Don't use often!
  std::string str() const { return std::string(str_begin, str_end); }

  TKind getKind() const { return kind; }

  bool isNumber() const {
    return kind == TKind::Number || kind == TKind::HexNumber;
  }

  int getNum() const {
    assert(isNumber() && "Not a number!");
    return num;
  }

  const char *getStrBegin() const {
    assert(kind == TKind::String && "Not a string!");
    return str_begin;
  }

  const char *getStrEnd() const {
    assert(kind == TKind::String && "Not a string!");
    return str_end;
  }

  Loc getStart() const { return start; }
  Loc getEnd() const { return end; }

  operator bool() const { return kind != TKind::Invalid; }

private:
  int num;
  const char *str_begin;
  const char *str_end;

  Loc start;
  Loc end;

  TKind kind = TKind::Invalid;
};

static int intFromHex(const char c) {
  return (c >= 'a') ? (c - 'a' + 10)
                    : ((c >= 'A') ? (c - 'A' + 10) : (c - '0'));
}

struct Lexer {
  Lexer(const std::string &file) : ptr(file.begin()), end(file.end()) {}

  Lexer(const Lexer &other) = delete;
  Lexer &operator==(const Lexer &other) = delete;

  void consume() {
    ++ptr;
    ++col;
  }

  bool skipSpace(SkipNewLine skip) {
    bool in_comment = false;
    while (ptr != end && (*ptr == '#' || std::isspace(*ptr) || in_comment)) {
      if (*ptr == '#') {
        in_comment = true;
      } else if (*ptr == '\n') {
        in_comment = false;
        if (skip == SkipNewLine::False) {
          return true;
        }
        ++line;
        col = 0; // Will be incremented to 1 below...
      }
      consume();
    }
    return ptr == end;
  }

  Tok lex(SkipNewLine skip = SkipNewLine::True) {
    Tok tok;
    tok.str_begin = &*ptr;
    tok.start = Loc(line, col);

    if (skipSpace(skip)) {
      tok.str_end = &*ptr;
      tok.end = Loc(line, col);
      if (*ptr == '\n') {
        tok.kind = TKind::NewLine;
      } else {
        tok.kind = TKind::EndOfFile;
      }
      return tok;
    }

    tok.str_begin = &*ptr;
    tok.start = Loc(line, col);

    // String
    if (std::isalpha(*ptr)) {
      while (ptr != end && std::isalnum(*ptr)) {
        consume();
      }
      tok.kind = TKind::String;
    } else if (std::isdigit(*ptr)) {
      // Number
      bool is_hex = false;
      if (*ptr == '0') {
        consume();
        if (*ptr == 'x') {
          consume();
          is_hex = true;
        }
      }

      int num = 0;
      if (!is_hex) {
        while (ptr != end && std::isdigit(*ptr)) {
          int digit = *ptr - '0';
          num = num * 10 + digit;
          consume();
        }
      } else {
        while (ptr != end && std::isxdigit(*ptr)) {
          const int int_val = intFromHex(*ptr);
          num = (num << 4) | (int_val & 0xF);
          consume();
        }
      }

      if (ptr != end && !std::isspace(*ptr)) {
        tok.str_end = &*ptr;
        tok.kind = TKind::Invalid;
        tok.end = Loc(line, col);
        if (!is_hex && std::isxdigit(*ptr)) {
          std::cout
              << tok.end
              << ": was not expecting a hex digit (try prefixing with 0x)\n";
        }
        return tok;
      }

      tok.num = num;
      tok.kind = !is_hex ? TKind::Number : TKind::HexNumber;
    } else {
      // Unrecognized token
      tok.end = Loc(line, col);
      tok.kind = TKind::Invalid;
      std::cout << tok.end << ": unrecognized token\n";
    }

    tok.str_end = &*ptr;
    tok.end = Loc(line, col);

    return tok;
  }

private:
  int col = 1;
  int line = 1;
  std::string::const_iterator ptr;
  const std::string::const_iterator end;
};

#endif // COLUMBO_LEXER_H
