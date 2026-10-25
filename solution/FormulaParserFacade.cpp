#include "FormulaParserFacade.h"

#include "antlr4-runtime.h"
#include "FormulaLexer.h"
#include "FormulaParser.h"
#include "FormulaBaseListener.h"

#include <cmath>
#include <exception>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class FormulaSyntaxErrorListener : public antlr4::BaseErrorListener {
public:
    void syntaxError(antlr4::Recognizer*, antlr4::Token*, size_t line, size_t column, const std::string& message, std::exception_ptr) override {
        const std::string location = std::to_string(line) + ":" + std::to_string(column);

        throw FormulaException("Formula syntax error at " + location + ": " + message);
    }
};

class FormulaBuilderListener : public FormulaBaseListener {
public:
    void exitLiteral(FormulaParser::LiteralContext* ctx) override {
        const std::string text = ctx->NUMBER()->getText();

        std::istringstream input(text);
        input.imbue(std::locale::classic());

        double value = 0.0;

        if (!(input >> std::noskipws >> value) || !input.eof() || !std::isfinite(value)) {
            throw FormulaException("Invalid or out-of-range number: " + text);
        }

        nodes_.push_back(std::make_unique<ASTImpl::NumberExpr>(value, text));
    }

    void exitCell(FormulaParser::CellContext* ctx) override {
        const std::string address = ctx->CELL()->getText();
        const Position position = Position::FromString(address);

        nodes_.push_back(std::make_unique<ASTImpl::CellExpr>(position, address));
    }

    void exitUnaryOp(FormulaParser::UnaryOpContext* ctx) override {
        const std::string operation_text = ctx->children.at(0)->getText();

        ASTImpl::UnaryOperation operation;

        if (operation_text == "+") {
            operation = ASTImpl::UnaryOperation::Plus;
        }
        else if (operation_text == "-") {
            operation = ASTImpl::UnaryOperation::Minus;
        }
        else {
            throw std::logic_error("Unsupported unary operation");
        }

        auto operand = PopNode();
        auto node = std::make_unique<ASTImpl::UnaryExpr>(operation, std::move(operand));

        nodes_.push_back(std::move(node));
    }

    void exitBinaryOp(FormulaParser::BinaryOpContext* ctx) override {
        const std::string operation_text = ctx->children.at(1)->getText();

        ASTImpl::BinaryOperation operation;

        if (operation_text == "+") {
            operation = ASTImpl::BinaryOperation::Add;
        }
        else if (operation_text == "-") {
            operation = ASTImpl::BinaryOperation::Subtract;
        }
        else if (operation_text == "*") {
            operation = ASTImpl::BinaryOperation::Multiply;
        }
        else if (operation_text == "/") {
            operation = ASTImpl::BinaryOperation::Divide;
        }
        else {
            throw std::logic_error("Unsupported binary operation");
        }

        auto right = PopNode();
        auto left = PopNode();

        auto node = std::make_unique<ASTImpl::BinaryExpr>(operation, std::move(left), std::move(right));

        nodes_.push_back(std::move(node));
    }

    std::unique_ptr<ASTImpl::Expr> TakeRoot() {
        if (nodes_.size() != 1) {
            throw std::logic_error("Expected one root node");
        }

        return PopNode();
    }

private:
    std::unique_ptr<ASTImpl::Expr> PopNode() {
        if (nodes_.empty()) {
            throw std::logic_error("Expression stack is empty");
        }

        auto node = std::move(nodes_.back());
        nodes_.pop_back();

        return node;
    }

    std::vector<std::unique_ptr<ASTImpl::Expr>> nodes_;
};

std::unique_ptr<ASTImpl::Expr> BuildExpression(const std::string& expression) {
    FormulaSyntaxErrorListener error_listener;

    antlr4::ANTLRInputStream input(expression);
    FormulaLexer lexer(&input);

    lexer.removeErrorListeners();
    lexer.addErrorListener(&error_listener);

    antlr4::CommonTokenStream tokens(&lexer);
    FormulaParser parser(&tokens);

    parser.removeErrorListeners();
    parser.addErrorListener(&error_listener);

    auto* tree = parser.main();

    FormulaBuilderListener builder;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&builder, tree);

    return builder.TakeRoot();
}