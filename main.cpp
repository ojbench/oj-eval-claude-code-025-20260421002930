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
    set<string> keywords;

    bool isKeyword(const string& s) {
        return keywords.count(s) > 0;
    }

    string renameVar(const string& var) {
        // Don't rename keywords
        if (isKeyword(var)) return var;
        // Generate unique variable names
        return "_x" + to_string(counter++) + "_" + var.substr(0, min((size_t)3, var.size()));
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
    Cheater() : rng(12345), counter(0) {
        // Initialize keywords set
        keywords = {"function", "block", "set", "if", "while", "print",
                   "array.create", "array.get", "array.set",
                   "+", "-", "*", "/", "=", "<", ">", "<=", ">=", "!=",
                   "and", "or", "not", "return"};
    }

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
    // Extract structural features from AST
    vector<string> extractFeatures(shared_ptr<SExpr> expr) {
        vector<string> features;
        if (!expr) return features;

        if (expr->type == SExpr::LIST && expr->children.size() > 0) {
            if (expr->children[0]->type == SExpr::ATOM) {
                features.push_back("KEYWORD:" + expr->children[0]->atom);
                features.push_back("SIZE:" + to_string(expr->children.size()));
            }
            for (auto& child : expr->children) {
                auto childFeatures = extractFeatures(child);
                features.insert(features.end(), childFeatures.begin(), childFeatures.end());
            }
        }
        return features;
    }

    double computeSimilarity(const vector<shared_ptr<SExpr>>& prog1,
                            const vector<shared_ptr<SExpr>>& prog2) {
        // Extract features from both programs
        vector<string> features1, features2;
        for (auto& expr : prog1) {
            auto f = extractFeatures(expr);
            features1.insert(features1.end(), f.begin(), f.end());
        }
        for (auto& expr : prog2) {
            auto f = extractFeatures(expr);
            features2.insert(features2.end(), f.begin(), f.end());
        }

        // Compare feature sets
        set<string> set1(features1.begin(), features1.end());
        set<string> set2(features2.begin(), features2.end());

        vector<string> intersection;
        set_intersection(set1.begin(), set1.end(),
                        set2.begin(), set2.end(),
                        back_inserter(intersection));

        if (set1.empty() && set2.empty()) return 0.5;

        double jaccard = (double)intersection.size() /
                        (set1.size() + set2.size() - intersection.size());

        // Also do structural comparison
        if (prog1.size() != prog2.size()) {
            jaccard *= 0.8;
        }

        int similarities = 0;
        int total = 0;
        for (size_t i = 0; i < min(prog1.size(), prog2.size()); i++) {
            auto score = compareExpr(prog1[i], prog2[i]);
            similarities += score.first;
            total += score.second;
        }

        double structural = total > 0 ? (double)similarities / total : 0.5;

        // Combine both metrics
        return 0.6 * jaccard + 0.4 * structural;
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
    // Try to read the first program
    string prog1 = readProgram(cin);

    // Try to read a second program
    string prog2 = readProgram(cin);

    if (prog2.empty()) {
        // Only one program provided - cheat mode
        Cheater cheater;
        cout << cheater.cheat(prog1) << endl;
    } else {
        // Two programs provided - anticheat mode
        string testInput;
        string line;
        while (getline(cin, line)) {
            testInput += line + "\n";
        }

        AntiCheater antiCheater;
        double similarity = antiCheater.detect(prog1, prog2, testInput);
        cout << similarity << endl;
    }

    return 0;
}
