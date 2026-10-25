#pragma once

#include "common.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace ASTImpl {

    class Expr {
    public:
        virtual ~Expr() = default;

        virtual double Evaluate(const SheetInterface& sheet) const = 0;
        virtual int GetPrecedence() const = 0;
        virtual void Print(std::ostream& output) const = 0;

        virtual void CollectReferencedCells(std::vector<Position>& /* cells */) const {
        }
    };

    class NumberExpr final : public Expr {
    public:
        explicit NumberExpr(double value, std::string text = {});

        double Evaluate(const SheetInterface& sheet) const override;
        int GetPrecedence() const override;
        void Print(std::ostream& output) const override;

    private:
        double value_;
        std::string text_;
    };

    class CellExpr final : public Expr {
    public:
        explicit CellExpr(Position position, std::string address = {});

        double Evaluate(const SheetInterface& sheet) const override;
        int GetPrecedence() const override;
        void Print(std::ostream& output) const override;
        void CollectReferencedCells(std::vector<Position>& cells) const override;

    private:
        Position position_;
        std::string address_;
    };

    enum class UnaryOperation {
        Plus,
        Minus
    };

    class UnaryExpr final : public Expr {
    public:
        UnaryExpr(UnaryOperation operation, std::unique_ptr<Expr> operand);

        double Evaluate(const SheetInterface& sheet) const override;
        int GetPrecedence() const override;
        void Print(std::ostream& output) const override;
        void CollectReferencedCells(std::vector<Position>& cells) const override;

    private:
        UnaryOperation operation_;
        std::unique_ptr<Expr> operand_;
    };

    enum class BinaryOperation {
        Add,
        Subtract,
        Multiply,
        Divide
    };

    class BinaryExpr final : public Expr {
    public:
        BinaryExpr(BinaryOperation operation, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right);

        double Evaluate(const SheetInterface& sheet) const override;
        int GetPrecedence() const override;
        void Print(std::ostream& output) const override;
        void CollectReferencedCells(std::vector<Position>& cells) const override;

    private:
        BinaryOperation operation_;
        std::unique_ptr<Expr> left_;
        std::unique_ptr<Expr> right_;
    };

} // namespace ASTImpl