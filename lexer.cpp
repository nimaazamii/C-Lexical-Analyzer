#include <cctype>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std;

namespace {
const set<string> keywords = {
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "inline", "int", "long", "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while", "_Alignas",
    "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
    "_Noreturn", "_Static_assert", "_Thread_local"
};

const vector<string> operators = {
    "%:%:", ">>=", "<<=", "...", "++", "--", "->", "&&", "||",
    "<=", ">=", "==", "!=", "*=", "/=", "%=", "+=", "-=", "<<",
    ">>", "&=", "^=", "|=", "##", "+", "-", "*", "/", "%", "=",
    "<", ">", "&", "|", "^", "!", "~", "?"
};

const vector<string> punctuators = {
    "[", "]", "(", ")", "{", "}", ".", ",", ";", ":", "#"
};

bool isIdentifierStart(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return isalpha(uc) || c == '_';
}

bool isIdentifierPart(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return isalnum(uc) || c == '_';
}

string escapeForOutput(const string& value) {
    string result;
    for (char c : value) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\\': result += "\\\\"; break;
            default: result += c; break;
        }
    }
    return result;
}

bool isHex(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return isxdigit(uc) != 0;
}

bool allDigits(const string& s, size_t begin, size_t end, int base) {
    if (begin >= end) return false;
    for (size_t i = begin; i < end; ++i) {
        const char c = s[i];
        if (base == 10 && !isdigit(static_cast<unsigned char>(c))) return false;
        if (base == 8 && !(c >= '0' && c <= '7')) return false;
        if (base == 16 && !isHex(c)) return false;
    }
    return true;
}

bool validIntegerSuffix(const string& suffix) {
    if (suffix.empty()) return true;
    string lower;
    for (char c : suffix) lower += static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return lower == "u" || lower == "l" || lower == "ll" ||
           lower == "ul" || lower == "lu" || lower == "ull" ||
           lower == "llu";
}

bool validFloatSuffix(const string& suffix) {
    return suffix.empty() || suffix == "f" || suffix == "F" ||
           suffix == "l" || suffix == "L";
}

bool isValidNumber(const string& token, string& type) {
    // Integer suffix is separated before checking the numeric body.
    size_t suffixStart = token.size();
    while (suffixStart > 0) {
        char c = token[suffixStart - 1];
        if (c == 'u' || c == 'U' || c == 'l' || c == 'L') --suffixStart;
        else break;
    }
    // f/F is a floating suffix only when the token has a floating marker.
    // This avoids treating the final F in a hexadecimal integer (0xFF) as a suffix.
    if (suffixStart == token.size() && suffixStart > 0 &&
        (token.back() == 'f' || token.back() == 'F') &&
        token.find_first_of(".eEpP") != string::npos) {
        --suffixStart;
    }
    const string body = token.substr(0, suffixStart);
    const string suffix = token.substr(suffixStart);

    // Decimal and hexadecimal floating-point forms supported by this lexer.
    const bool hexadecimal = body.size() >= 2 && body[0] == '0' &&
                             (body[1] == 'x' || body[1] == 'X');
    bool hasFloatMarker = body.find('.') != string::npos;
    if (hexadecimal) {
        hasFloatMarker = hasFloatMarker ||
                         body.find('p') != string::npos ||
                         body.find('P') != string::npos;
    } else {
        hasFloatMarker = hasFloatMarker ||
                         body.find('e') != string::npos ||
                         body.find('E') != string::npos;
    }

    if (hasFloatMarker) {
        if (!validFloatSuffix(suffix)) return false;
        if (body.empty()) return false;

        // Decimal floating-point: 12.3, .5, 12., 1e-3, 1.2e+3
        size_t i = 0;
        bool digitsBefore = false, digitsAfter = false;
        while (i < body.size() && isdigit(static_cast<unsigned char>(body[i]))) {
            digitsBefore = true; ++i;
        }
        if (i < body.size() && body[i] == '.') {
            ++i;
            while (i < body.size() && isdigit(static_cast<unsigned char>(body[i]))) {
                digitsAfter = true; ++i;
            }
        }
        if (!digitsBefore && !digitsAfter) return false;
        if (i < body.size() && (body[i] == 'e' || body[i] == 'E')) {
            ++i;
            if (i < body.size() && (body[i] == '+' || body[i] == '-')) ++i;
            size_t expStart = i;
            while (i < body.size() && isdigit(static_cast<unsigned char>(body[i]))) ++i;
            if (i == expStart) return false;
        }
        if (i == body.size()) {
            type = "FLOAT";
            return true;
        }

        // Hexadecimal floating-point requires a binary exponent.
        if (body.size() >= 3 && body[0] == '0' &&
            (body[1] == 'x' || body[1] == 'X')) {
            size_t p = body.find_first_of("pP", 2);
            if (p == string::npos) return false;
            string mantissa = body.substr(2, p - 2);
            bool mantissaDigit = false;
            for (char c : mantissa) {
                if (c == '.') continue;
                if (!isHex(c)) return false;
                mantissaDigit = true;
            }
            if (!mantissaDigit) return false;
            size_t e = p + 1;
            if (e < body.size() && (body[e] == '+' || body[e] == '-')) ++e;
            if (!allDigits(body, e, body.size(), 10)) return false;
            type = "FLOAT";
            return true;
        }
        return false;
    }

    if (!validIntegerSuffix(suffix)) return false;
    if (body.empty()) return false;

    if (body.size() >= 2 && body[0] == '0' &&
        (body[1] == 'x' || body[1] == 'X')) {
        if (!allDigits(body, 2, body.size(), 16)) return false;
        type = "HEX_INTEGER";
        return true;
    }
    if (body.size() > 1 && body[0] == '0') {
        if (!allDigits(body, 1, body.size(), 8)) return false;
        type = "OCTAL_INTEGER";
        return true;
    }
    if (!allDigits(body, 0, body.size(), 10)) return false;
    type = "DECIMAL_INTEGER";
    return true;
}

struct Lexer {
    string source;
    size_t pos = 0;
    int line = 1;
    int column = 1;
    ofstream& output;

    Lexer(string input, ofstream& out) : source(std::move(input)), output(out) {}

    bool eof() const { return pos >= source.size(); }
    char peek(size_t offset = 0) const {
        return pos + offset < source.size() ? source[pos + offset] : '\0';
    }

    char take() {
        if (eof()) return '\0';
        char c = source[pos++];
        if (c == '\n') { ++line; column = 1; }
        else ++column;
        return c;
    }

    void emit(const string& type, const string& value, int tokenLine, int tokenColumn) {
        const string rendered = escapeForOutput(value);
        cout << type << " : " << rendered << " (" << tokenLine << ":" << tokenColumn << ")\n";
        output << type << " : " << rendered << " (" << tokenLine << ":" << tokenColumn << ")\n";
    }

    void emitInvalid(const string& value, int tokenLine, int tokenColumn) {
        emit("INVALID", value, tokenLine, tokenColumn);
    }

    void scan() {
        while (!eof()) {
            const int tokenLine = line;
            const int tokenColumn = column;
            char c = peek();

            if (isspace(static_cast<unsigned char>(c))) {
                string value;
                while (!eof() && isspace(static_cast<unsigned char>(peek()))) value += take();
                emit("WHITESPACE", value, tokenLine, tokenColumn);
                continue;
            }

            if (isIdentifierStart(c)) {
                string value;
                while (!eof() && isIdentifierPart(peek())) value += take();
                emit(keywords.count(value) ? "KEYWORD" : "IDENTIFIER",
                     value, tokenLine, tokenColumn);
                continue;
            }

            if (isdigit(static_cast<unsigned char>(c)) ||
                (c == '.' && isdigit(static_cast<unsigned char>(peek(1))))) {
                string value;
                while (!eof()) {
                    char n = peek();
                    if (isalnum(static_cast<unsigned char>(n)) || n == '_' || n == '.') {
                        value += take();
                    } else if ((n == '+' || n == '-') && !value.empty() &&
                               (value.back() == 'e' || value.back() == 'E' ||
                                value.back() == 'p' || value.back() == 'P')) {
                        value += take();
                    } else break;
                }
                string type;
                if (isValidNumber(value, type)) emit(type, value, tokenLine, tokenColumn);
                else emitInvalid(value, tokenLine, tokenColumn);
                continue;
            }

            if (c == '"' || c == '\'') {
                const char quote = take();
                string value(1, quote);
                bool closed = false;
                bool valid = true;
                while (!eof()) {
                    char n = take();
                    value += n;
                    if (n == '\n') { valid = false; break; }
                    if (n == '\\') {
                        if (eof()) { valid = false; break; }
                        char escaped = take();
                        value += escaped;
                        if (escaped == '\n') valid = false;
                        continue;
                    }
                    if (n == quote) { closed = true; break; }
                }
                if (closed && valid) {
                    if (quote == '"') emit("STRING", value, tokenLine, tokenColumn);
                    else {
                        // A character constant must contain one character or one escape sequence.
                        size_t contentLength = value.size() - 2;
                        bool oneCharacter = contentLength == 1 ||
                            (contentLength == 2 && value[1] == '\\');
                        if (oneCharacter) emit("CHARACTER", value, tokenLine, tokenColumn);
                        else emitInvalid(value, tokenLine, tokenColumn);
                    }
                } else emitInvalid(value, tokenLine, tokenColumn);
                continue;
            }

            if (c == '/' && (peek(1) == '/' || peek(1) == '*')) {
                string value;
                value += take();
                const char kind = take();
                value += kind;
                if (kind == '/') {
                    while (!eof() && peek() != '\n') value += take();
                } else {
                    bool closed = false;
                    while (!eof()) {
                        char n = take();
                        value += n;
                        if (n == '*' && peek() == '/') {
                            value += take();
                            closed = true;
                            break;
                        }
                    }
                    if (!closed) {
                        emitInvalid(value, tokenLine, tokenColumn);
                        continue;
                    }
                }
                emit("COMMENT", value, tokenLine, tokenColumn);
                continue;
            }

            bool matched = false;
            for (const string& op : operators) {
                if (source.compare(pos, op.size(), op) == 0) {
                    for (size_t i = 0; i < op.size(); ++i) take();
                    emit("OPERATOR", op, tokenLine, tokenColumn);
                    matched = true;
                    break;
                }
            }
            if (matched) continue;

            for (const string& punct : punctuators) {
                if (source.compare(pos, punct.size(), punct) == 0) {
                    for (size_t i = 0; i < punct.size(); ++i) take();
                    emit("PUNCTUATOR", punct, tokenLine, tokenColumn);
                    matched = true;
                    break;
                }
            }
            if (matched) continue;

            string invalid(1, take());
            emitInvalid(invalid, tokenLine, tokenColumn);
        }
    }
};
} // namespace

int main(int argc, char* argv[]) {
    const string inputPath = argc > 1 ? argv[1] : "source.txt";
    const string outputPath = argc > 2 ? argv[2] : "output.txt";

    ifstream input(inputPath, ios::binary);
    if (!input) {
        cerr << "Error: cannot open input file '" << inputPath << "'.\n";
        return 1;
    }

    string source((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
    input.close();

    ofstream output(outputPath);
    if (!output) {
        cerr << "Error: cannot create output file '" << outputPath << "'.\n";
        return 1;
    }

    Lexer lexer(source, output);
    lexer.scan();
    return 0;
}
