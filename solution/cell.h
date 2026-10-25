#pragma once

#include "common.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet);
    ~Cell() override;

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    friend class Sheet;

    void InvalidateCache();
    void SwapContent(Cell& other) noexcept;

    class Impl {
    public:
        virtual ~Impl() = default;

        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
    };

    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cache_;
};