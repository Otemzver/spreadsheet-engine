#pragma once

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct Position {
    int row = 0;
    int col = 0;

    static constexpr int MAX_ROWS = 16384;
    static constexpr int MAX_COLS = 16384;

    static const Position NONE;

    bool IsValid() const;

    bool operator==(const Position& other) const;
    bool operator<(const Position& other) const;

    std::string ToString() const;

    static Position FromString(std::string_view text);
};

struct Size {
    int rows = 0;
    int cols = 0;

    bool operator==(const Size& other) const;
};

class FormulaError {
public:
    enum class Category {
        Ref,
        Value,
        Arithmetic
    };

    FormulaError(Category category)
        : category_(category) {
    }

    Category GetCategory() const {
        return category_;
    }

    bool operator==(const FormulaError& other) const {
        return category_ == other.category_;
    }

    std::string_view ToString() const {
        switch (category_) {
        case Category::Ref:
            return "#REF!";

        case Category::Value:
            return "#VALUE!";

        case Category::Arithmetic:
            return "#ARITHM!";
        }

        return "#ERROR!";
    }

private:
    Category category_;
};

inline constexpr char FORMULA_SIGN = '=';
inline constexpr char ESCAPE_SIGN = '\'';

class CellInterface {
public:
    using Value = std::variant<std::string, double, FormulaError>;

    virtual ~CellInterface() = default;

    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class InvalidPositionException :public std::out_of_range {
public:
    using std::out_of_range::out_of_range;
};

class SheetInterface {
public:
    virtual ~SheetInterface() = default;

    virtual void SetCell(Position pos, std::string text) = 0;

    virtual const CellInterface* GetCell(Position pos) const = 0;
    virtual CellInterface* GetCell(Position pos) = 0;

    virtual void ClearCell(Position pos) = 0;

    virtual Size GetPrintableSize() const = 0;

    virtual void PrintValues(std::ostream& output) const = 0;
    virtual void PrintTexts(std::ostream& output) const = 0;
};

std::unique_ptr<SheetInterface> CreateSheet();

class FormulaException :public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class CircularDependencyException : public std::logic_error {
public:
    using std::logic_error::logic_error;
};