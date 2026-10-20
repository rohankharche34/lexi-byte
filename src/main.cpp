
#include <cctype>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// ---------- 1. Lexer ----------

enum class TokenKind {
    Number, Plus, Minus, Star, Slash,
    LParen, RParen, End
};

struct Token {
    TokenKind kind;
    std::string text;
    std::size_t position;
};

class Lexer {
    const std::string& source;
    std::size_t pos = 0;

public:
    explicit Lexer(const std::string& input) : source(input) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;

        while (pos < source.size()) {
            unsigned char c =
                static_cast<unsigned char>(source[pos]);

            if (std::isspace(c)) {
                ++pos;
                continue;
            }

            const std::size_t start = pos;

            if (std::isdigit(c)) {
                while (pos < source.size() &&
                       std::isdigit(static_cast<unsigned char>(
                           source[pos]))) {
                    ++pos;
                }

                tokens.push_back({
                    TokenKind::Number,
                    source.substr(start, pos - start),
                    start
                });
                continue;
            }

            TokenKind kind;

            switch (source[pos]) {
                case '+': kind = TokenKind::Plus;  break;
                case '-': kind = TokenKind::Minus; break;
                case '*': kind = TokenKind::Star;  break;
                case '/': kind = TokenKind::Slash; break;
                case '(': kind = TokenKind::LParen; break;
                case ')': kind = TokenKind::RParen; break;
                default:
                    throw std::runtime_error(
                        "Invalid character at position " +
                        std::to_string(pos));
            }

            tokens.push_back({
                kind, source.substr(pos, 1), pos
            });
            ++pos;
        }

        tokens.push_back({
            TokenKind::End, "", source.size()
        });

        return tokens;
    }
};

// ---------- 2. Abstract Syntax Tree ----------

struct Node {
    bool isNumber;
    long long value = 0;
    char op = '\0';
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    explicit Node(long long v) : isNumber(true), value(v) {}

    Node(char operation,
         std::unique_ptr<Node> lhs,
         std::unique_ptr<Node> rhs)
        : isNumber(false), op(operation),
          left(std::move(lhs)), right(std::move(rhs)) {}
};

using NodePtr = std::unique_ptr<Node>;

NodePtr number(long long value) {
    return std::make_unique<Node>(value);
}

NodePtr binary(char op, NodePtr lhs, NodePtr rhs) {
    return std::make_unique<Node>(
        op, std::move(lhs), std::move(rhs));
}

// ---------- 3. Recursive-Descent Parser ----------
//
// expression := term (('+' | '-') term)*
// term       := factor (('*' | '/') factor)*
// factor     := NUMBER | '(' expression ')' | '-' factor

class Parser {
    const std::vector<Token>& tokens;
    std::size_t pos = 0;

    const Token& current() const {
        return tokens[pos];
    }

    bool match(TokenKind kind) {
        if (current().kind != kind) return false;
        ++pos;
        return true;
    }

    void expect(TokenKind kind, const std::string& message) {
        if (!match(kind)) {
            throw std::runtime_error(
                message + " at position " +
                std::to_string(current().position));
        }
    }

    NodePtr expression() {
        NodePtr lhs = term();

        while (current().kind == TokenKind::Plus ||
               current().kind == TokenKind::Minus) {
            char op = current().text[0];
            ++pos;
            lhs = binary(op, std::move(lhs), term());
        }

        return lhs;
    }

    NodePtr term() {
        NodePtr lhs = factor();

        while (current().kind == TokenKind::Star ||
               current().kind == TokenKind::Slash) {
            char op = current().text[0];
            ++pos;
            lhs = binary(op, std::move(lhs), factor());
        }

        return lhs;
    }

    NodePtr factor() {
        if (current().kind == TokenKind::Number) {
            std::string text = current().text;
            ++pos;

            try {
                return number(std::stoll(text));
            } catch (const std::out_of_range&) {
                throw std::runtime_error(
                    "Integer literal out of range");
            }
        }

        if (match(TokenKind::Minus)) {
            return binary('-', number(0), factor());
        }

        if (match(TokenKind::LParen)) {
            NodePtr result = expression();
            expect(TokenKind::RParen, "Expected ')'");
            return result;
        }

        throw std::runtime_error(
            "Expected a number or '(' at position " +
            std::to_string(current().position));
    }

public:
    explicit Parser(const std::vector<Token>& input)
        : tokens(input) {}

    NodePtr parse() {
        NodePtr root = expression();

        if (current().kind != TokenKind::End) {
            throw std::runtime_error(
                "Unexpected token '" + current().text +
                "' at position " +
                std::to_string(current().position));
        }

        return root;
    }
};

// ---------- 4. Constant-Folding Optimizer ----------

long long applyOperator(char op, long long a, long long b) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0)
                throw std::runtime_error("Division by zero");
            return a / b;
        default:
            throw std::runtime_error("Unknown operator");
    }
}

NodePtr optimize(NodePtr node) {
    if (node->isNumber) return node;

    node->left = optimize(std::move(node->left));
    node->right = optimize(std::move(node->right));

    if (node->left->isNumber &&
        node->right->isNumber) {
        long long a = node->left->value;
        long long b = node->right->value;

        // Preserve division by zero for evaluation to report.
        if (node->op == '/' && b == 0) return node;

        long long result =
            applyOperator(node->op, a, b);

        return number(result);
    }

    return node;
}

// ---------- 5. AST Printing and Evaluation ----------

void printAST(const Node& node) {
    if (node.isNumber) {
        std::cout << node.value;
        return;
    }

    std::cout << "(" << node.op << " ";
    printAST(*node.left);
    std::cout << " ";
    printAST(*node.right);
    std::cout << ")";
}

long long evaluate(const Node& node) {
    if (node.isNumber) return node.value;

    long long lhs = evaluate(*node.left);
    long long rhs = evaluate(*node.right);

    return applyOperator(node.op, lhs, rhs);
}

NodePtr compileExpression(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    return optimize(parser.parse());
}

// ---------- 6. Basic Tests ----------

void runTests() {
    int passed = 0;

    auto check = [&](const std::string& input,
                     long long expected) {
        auto ast = compileExpression(input);
        long long actual = evaluate(*ast);

        if (actual != expected)
            throw std::runtime_error(
                "Test failed: " + input);

        ++passed;
        std::cout << "[PASS] " << input << '\n';
    };

    auto checkError = [&](const std::string& input) {
        try {
            auto ast = compileExpression(input);
            (void)evaluate(*ast);
        } catch (const std::exception&) {
            ++passed;
            std::cout << "[PASS] Error detected: "
                      << input << '\n';
            return;
        }

        throw std::runtime_error(
            "Expected an error for: " + input);
    };

    check("2 + 3 * 4", 14);
    check("(2 + 3) * 4", 20);
    check("2 * 3 + 4", 10);
    check("-5 + 8", 3);
    check("(2 + 3) * (4 + 1)", 25);
    checkError("2 + * 3");
    checkError("1 / 0");

    std::cout << passed << " tests passed.\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 2 &&
            std::string(argv[1]) == "--test") {
            runTests();
            return 0;
        }

        std::string source;

        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                if (i > 1) source += ' ';
                source += argv[i];
            }
        } else {
            std::cout << "Expression: ";
            std::getline(std::cin, source);
        }

        auto ast = compileExpression(source);

        std::cout << "Optimized AST: ";
        printAST(*ast);
        std::cout << '\n';

        std::cout << "Result: "
                  << evaluate(*ast) << '\n';

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

