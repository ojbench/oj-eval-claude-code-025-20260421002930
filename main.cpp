#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <random>
#include <cctype>
#include <memory>

using namespace std;

// Simple tokenizer
struct Token {
    enum Type { LPAREN, RPAREN, SYMBOL, NUMBER, STRING, END };
    Type type;
    string value;
};

class Tokenizer {
private:
    string input;
    size_t pos;

public:
    Tokenizer(const string& s) : input(s), pos(0) {}

    void skipWhitespace() {
        while (pos < input.size() && isspace(input[pos])) pos++;
    }

    Token nextToken() {
        skipWhitespace();
        if (pos >= input.size()) return {Token::END, ""};

        if (input[pos] == '(') {
            pos++;
            return {Token::LPAREN, "("};
        }
        if (input[pos] == ')') {
            pos++;
            return {Token::RPAREN, ")"};
        }

        // Read symbol or number
        size_t start = pos;
        while (pos < input.size() && !isspace(input[pos]) &&
               input[pos] != '(' && input[pos] != ')') {
            pos++;
        }
        string value = input.substr(start, pos - start);

        // Check if it's a number
        bool isNum = true;
        for (char c : value) {
            if (!isdigit(c) && c != '-' && c != '.') {
                isNum = false;
                break;
            }
        }

        return {isNum ? Token::NUMBER : Token::SYMBOL, value};
    }
};

// S-expression AST
struct SExpr {
    enum Type { ATOM, LIST };
    Type type;
    string atom;
    vector<shared_ptr<SExpr>> children;

    SExpr(const string& a) : type(ATOM), atom(a) {}
    SExpr() : type(LIST) {}

    string toString() const {
        if (type == ATOM) return atom;
        string result = "(";
        for (size_t i = 0; i < children.size(); i++) {
            if (i > 0) result += " ";
            result += children[i]->toString();
        }
        result += ")";
        return result;
    }
};

// Parser
class Parser {
private:
    Tokenizer& tokenizer;
    Token current;

    void advance() {
        current = tokenizer.nextToken();
    }

public:
    Parser(Tokenizer& t) : tokenizer(t) {
        advance();
    }

    shared_ptr<SExpr> parse() {
        if (current.type == Token::END) return nullptr;
        if (current.type == Token::LPAREN) {
            advance();
            auto expr = make_shared<SExpr>();
            expr->type = SExpr::LIST;
            while (current.type != Token::RPAREN && current.type != Token::END) {
                auto child = parse();
                if (child) expr->children.push_back(child);
            }
            if (current.type == Token::RPAREN) advance();
            return expr;
        } else {
            auto expr = make_shared<SExpr>(current.value);
            advance();
            return expr;
        }
    }

    vector<shared_ptr<SExpr>> parseAll() {
        vector<shared_ptr<SExpr>> result;
        while (current.type != Token::END) {
            auto expr = parse();
            if (expr) result.push_back(expr);
        }
        return result;
    }
};

// Read program until "endprogram" marker
string readProgram(istream& in) {
    string result, line;
    while (getline(in, line)) {
        if (line == "endprogram") break;
        result += line + "\n";
    }
    return result;
}

// Cheat: Transform program to evade detection
class Cheater {
private:
    mt19937 rng;
    int counter;

    string renameVar(const string& var) {
        // Rename variables
        if (var == "function" || var == "block" || var == "set" ||
            var == "if" || var == "while" || var == "print" ||
            var == "array.create" || var == "array.get" || var == "array.set" ||
            var == "+", var == "-" || var == "*" || var == "/" ||
            var == "=" || var == "<" || var == ">" || var == "and" || var == "or") {
            return var; // Don't rename keywords
        }
        // Add prefix to user variables
        return "v" + to_string(counter++) + "_" + var;
    }

    shared_ptr<SExpr> transform(shared_ptr<SExpr> expr, map<string, string>& varMap) {
        if (!expr) return expr;

        if (expr->type == SExpr::ATOM) {
            if (varMap.count(expr->atom)) {
                return make_shared<SExpr>(varMap[expr->atom]);
            }
            return expr;
        }

        auto result = make_shared<SExpr>();
        result->type = SExpr::LIST;

        // Handle function definitions specially
        if (expr->children.size() > 0 &&
            expr->children[0]->type == SExpr::ATOM &&
            expr->children[0]->atom == "function") {

            result->children.push_back(expr->children[0]); // "function"

            // Process function name and parameters
            if (expr->children.size() > 1 && expr->children[1]->type == SExpr::LIST) {
                auto params = make_shared<SExpr>();
                params->type = SExpr::LIST;

                map<string, string> localVarMap = varMap;
                for (auto& param : expr->children[1]->children) {
                    string newName = renameVar(param->atom);
                    localVarMap[param->atom] = newName;
                    params->children.push_back(make_shared<SExpr>(newName));
                }
                result->children.push_back(params);

                // Transform body with local scope
                for (size_t i = 2; i < expr->children.size(); i++) {
                    result->children.push_back(transform(expr->children[i], localVarMap));
                }
            }
            return result;
        }

        // Handle set statements
        if (expr->children.size() > 0 &&
            expr->children[0]->type == SExpr::ATOM &&
            expr->children[0]->atom == "set") {

            result->children.push_back(expr->children[0]); // "set"

            if (expr->children.size() > 1 && expr->children[1]->type == SExpr::ATOM) {
                string varName = expr->children[1]->atom;
                string newName = renameVar(varName);
                varMap[varName] = newName;
                result->children.push_back(make_shared<SExpr>(newName));

                for (size_t i = 2; i < expr->children.size(); i++) {
                    result->children.push_back(transform(expr->children[i], varMap));
                }
            }
            return result;
        }

        // Default: recursively transform all children
        for (auto& child : expr->children) {
            result->children.push_back(transform(child, varMap));
        }
        return result;
    }

public:
    Cheater() : rng(12345), counter(0) {}

    string cheat(const string& program) {
        Tokenizer tokenizer(program);
        Parser parser(tokenizer);
        auto exprs = parser.parseAll();

        map<string, string> varMap;
        string result;
        for (auto& expr : exprs) {
            auto transformed = transform(expr, varMap);
            result += transformed->toString() + "\n";
        }
        return result;
    }
};

// Anticheat: Detect plagiarism
class AntiCheater {
private:
    double computeSimilarity(const vector<shared_ptr<SExpr>>& prog1,
                            const vector<shared_ptr<SExpr>>& prog2) {
        // Simple structural comparison
        if (prog1.size() != prog2.size()) {
            return 0.3 + 0.2 * (1.0 - abs((int)prog1.size() - (int)prog2.size()) /
                                (double)max(prog1.size(), prog2.size()));
        }

        int similarities = 0;
        int total = 0;

        for (size_t i = 0; i < prog1.size(); i++) {
            auto score = compareExpr(prog1[i], prog2[i]);
            similarities += score.first;
            total += score.second;
        }

        if (total == 0) return 0.5;
        return (double)similarities / total;
    }

    pair<int, int> compareExpr(shared_ptr<SExpr> e1, shared_ptr<SExpr> e2) {
        if (!e1 || !e2) return {0, 1};

        if (e1->type != e2->type) return {0, 1};

        if (e1->type == SExpr::ATOM) {
            // Check if atoms are the same or both are keywords
            bool isKeyword1 = (e1->atom == "function" || e1->atom == "block" ||
                              e1->atom == "set" || e1->atom == "if");
            bool isKeyword2 = (e2->atom == "function" || e2->atom == "block" ||
                              e2->atom == "set" || e2->atom == "if");

            if (isKeyword1 && isKeyword2 && e1->atom == e2->atom) {
                return {1, 1};
            }
            if (e1->atom == e2->atom) {
                return {1, 1};
            }
            return {0, 1};
        }

        // LIST type
        if (e1->children.size() != e2->children.size()) {
            int minSize = min(e1->children.size(), e2->children.size());
            int maxSize = max(e1->children.size(), e2->children.size());
            int sim = 0, tot = 0;
            for (int i = 0; i < minSize; i++) {
                auto score = compareExpr(e1->children[i], e2->children[i]);
                sim += score.first;
                tot += score.second;
            }
            tot += (maxSize - minSize);
            return {sim, tot};
        }

        int sim = 0, tot = 0;
        for (size_t i = 0; i < e1->children.size(); i++) {
            auto score = compareExpr(e1->children[i], e2->children[i]);
            sim += score.first;
            tot += score.second;
        }
        return {sim, tot};
    }

public:
    double detect(const string& prog1, const string& prog2, const string& testInput) {
        Tokenizer t1(prog1);
        Parser p1(t1);
        auto exprs1 = p1.parseAll();

        Tokenizer t2(prog2);
        Parser p2(t2);
        auto exprs2 = p2.parseAll();

        return computeSimilarity(exprs1, exprs2);
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <cheat|anticheat>" << endl;
        return 1;
    }

    string mode = argv[1];

    if (mode == "cheat") {
        string program = readProgram(cin);
        Cheater cheater;
        cout << cheater.cheat(program) << endl;
    } else if (mode == "anticheat") {
        string prog1 = readProgram(cin);
        string prog2 = readProgram(cin);
        string testInput;
        getline(cin, testInput); // Read remaining input

        AntiCheater antiCheater;
        double similarity = antiCheater.detect(prog1, prog2, testInput);
        cout << similarity << endl;
    } else {
        cerr << "Invalid mode. Use 'cheat' or 'anticheat'" << endl;
        return 1;
    }

    return 0;
}
