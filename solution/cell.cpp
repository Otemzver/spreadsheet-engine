#include "cell.h"
#include "formula.h"

#include <utility>
#include <variant>

class Cell::EmptyImpl final : public Cell::Impl {
public:
    Value GetValue() const override {
        return std::string{};
    }

    std::string GetText() const override {
        return {};
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
};

class Cell::TextImpl final : public Cell::Impl {
public:
    explicit TextImpl(std::string text) : text_(std::move(text)) {
    }

    Value GetValue() const override {
        if (!text_.empty() && text_.front() == ESCAPE_SIGN) {
            return text_.substr(1);
        }

        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }

private:
    std::string text_;
};

class Cell::FormulaImpl final : public Cell::Impl {
public:
    FormulaImpl(std::string expression, const SheetInterface& sheet)
        : sheet_(sheet), formula_(ParseFormula(std::move(expression))) {
    }

    Value GetValue() const override {
        const auto result = formula_->Evaluate(sheet_);

        if (const auto* number = std::get_if<double>(&result)) {
            return *number;
        }

        return std::get<FormulaError>(result);
    }

    std::string GetText() const override {
        return "=" + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

private:
    const SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet), impl_(std::make_unique<EmptyImpl>()) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text.front() == '=') {
        impl_ = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
    }
    else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }

    InvalidateCache();
}

void Cell::InvalidateCache() {
    cache_.reset();
}

void Cell::SwapContent(Cell& other) noexcept {
    impl_.swap(other.impl_);
    cache_.reset();
    other.cache_.reset();
}

void Cell::Clear() {
    Set("");
}

Cell::Value Cell::GetValue() const {
    if (!cache_) {
        cache_ = impl_->GetValue();
    }

    return *cache_;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}