#include "FormulaAST.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <variant>

namespace ASTImpl {
    namespace {

        double CheckedNumber(double value) {
            if (!std::isfinite(value)) {
                throw FormulaError(FormulaError::Category::Arithmetic);
            }

            return value;
        }

        void PrintChild(std::ostream& output, const Expr& child, bool parentheses) {
            if (parentheses) {
                output << '(';
            }

            child.Print(output);

            if (parentheses) {
                output << ')';
            }
        }

    } // namespace

    NumberExpr::NumberExpr(double value, std::string text)
        : value_(value), text_(std::move(text)) {
        if (!std::isfinite(value_)) {
            throw FormulaException("Number is outside the supported range");
        }

        if (text_.empty()) {
            std::ostringstream output;
            output.imbue(std::locale::classic());
            output << std::setprecision(std::numeric_limits<double>::max_digits10) << value_;

            text_ = output.str();
        }
    }

    double NumberExpr::Evaluate(const SheetInterface&) const {
        return value_;
    }

    int NumberExpr::GetPrecedence() const {
        return value_ < 0.0 ? 3 : 4;
    }

    void NumberExpr::Print(std::ostream& output) const {
        output << text_;
    }

    CellExpr::CellExpr(Position position, std::string address)
        : position_(position), address_(std::move(address)) {
        if (address_.empty()) {
            address_ = position_.IsValid() ? position_.ToString() : "A0";
        }
    }

    double CellExpr::Evaluate(const SheetInterface& sheet) const {
        if (!position_.IsValid()) {
            throw FormulaError(FormulaError::Category::Ref);
        }

        const CellInterface* cell = sheet.GetCell(position_);

        if (cell == nullptr) {
            return 0.0;
        }

        const auto value = cell->GetValue();

        if (const auto* number = std::get_if<double>(&value)) {
            return CheckedNumber(*number);
        }

        if (const auto* error = std::get_if<FormulaError>(&value)) {
            throw* error;
        }

        const std::string& text = std::get<std::string>(value);

        if (text.empty()) {
            return 0.0;
        }

        std::istringstream input(text);
        input.imbue(std::locale::classic());

        double number = 0.0;

        if (!(input >> std::noskipws >> number) || !input.eof() || !std::isfinite(number)) {
            throw FormulaError(FormulaError::Category::Value);
        }

        return number;
    }

    int CellExpr::GetPrecedence() const {
        return 4;
    }

    void CellExpr::Print(std::ostream& output) const {
        output << address_;
    }

    void CellExpr::CollectReferencedCells(std::vector<Position>& cells) const {
        if (position_.IsValid()) {
            cells.push_back(position_);
        }
    }

    UnaryExpr::UnaryExpr(UnaryOperation operation, std::unique_ptr<Expr> operand)
        : operation_(operation), operand_(std::move(operand)) {
    }

    double UnaryExpr::Evaluate(const SheetInterface& sheet) const {
        const double value = operand_->Evaluate(sheet);

        switch (operation_) {
        case UnaryOperation::Plus:
            return CheckedNumber(value);

        case UnaryOperation::Minus:
            return CheckedNumber(-value);
        }

        throw std::logic_error("Unknown unary operation");
    }

    int UnaryExpr::GetPrecedence() const {
        return 3;
    }

    void UnaryExpr::Print(std::ostream& output) const {
        output << (operation_ == UnaryOperation::Plus ? '+' : '-');

        PrintChild(output, *operand_, operand_->GetPrecedence() < GetPrecedence());
    }

    void UnaryExpr::CollectReferencedCells(std::vector<Position>& cells) const {
        operand_->CollectReferencedCells(cells);
    }

    BinaryExpr::BinaryExpr(BinaryOperation operation, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : operation_(operation), left_(std::move(left)), right_(std::move(right)) {
    }

    double BinaryExpr::Evaluate(const SheetInterface& sheet) const {
        const double left = left_->Evaluate(sheet);
        const double right = right_->Evaluate(sheet);

        switch (operation_) {
        case BinaryOperation::Add:
            return CheckedNumber(left + right);

        case BinaryOperation::Subtract:
            return CheckedNumber(left - right);

        case BinaryOperation::Multiply:
            return CheckedNumber(left * right);

        case BinaryOperation::Divide:
            if (right == 0.0) {
                throw FormulaError(FormulaError::Category::Arithmetic);
            }

            return CheckedNumber(left / right);
        }

        throw std::logic_error("Unknown binary operation");
    }

    int BinaryExpr::GetPrecedence() const {
        return operation_ == BinaryOperation::Add || operation_ == BinaryOperation::Subtract ? 1 : 2;
    }

    void BinaryExpr::Print(std::ostream& output) const {
        const int precedence = GetPrecedence();

        PrintChild(output, *left_, left_->GetPrecedence() < precedence);

        switch (operation_) {
        case BinaryOperation::Add:
            output << '+';
            break;

        case BinaryOperation::Subtract:
            output << '-';
            break;

        case BinaryOperation::Multiply:
            output << '*';
            break;

        case BinaryOperation::Divide:
            output << '/';
            break;
        }

        PrintChild(output, *right_, right_->GetPrecedence() <= precedence);
    }

    void BinaryExpr::CollectReferencedCells(std::vector<Position>& cells) const {
        left_->CollectReferencedCells(cells);
        right_->CollectReferencedCells(cells);
    }

} // namespace ASTImpl