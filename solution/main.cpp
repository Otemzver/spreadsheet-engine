#include "common.h"

#include "antlr4-runtime.h"
#include "FormulaLexer.h"
#include "FormulaParser.h"
#include "FormulaBaseListener.h"
#include "FormulaAST.h"
#include "FormulaParserFacade.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <utility>
#include "cell.h"
#include "formula.h"
#include <vector>
#include <stdexcept>
#include <sstream>


void TestPositionValidity() {
    assert((Position{ 0, 0 }.IsValid()));
    assert((Position{ 16383, 16383 }.IsValid()));

    assert((!Position{ -1, 0 }.IsValid()));
    assert((!Position{ 0, -1 }.IsValid()));

    assert((!Position{ Position::MAX_ROWS, 0 }.IsValid()));
    assert((!Position{ 0, Position::MAX_COLS }.IsValid()));

    assert(!Position::NONE.IsValid());

    std::cout << "TestPositionValidity OK\n";
}

void TestPositionComparison() {
    assert((Position{ 4, 2 } == Position{ 4, 2 }));

    assert((!(Position{ 4, 2 } == Position{ 4, 3 })));
    assert((!(Position{ 4, 2 } == Position{ 5, 2 })));

    // В одной строке сравниваем столбцы.
    assert((Position{ 0, 0 } < Position{ 0, 1 }));

    // При разных строках столбцы не определяют порядок.
    assert((Position{ 0, 25 } < Position{ 1, 0 }));
    assert((!(Position{ 1, 0 } < Position{ 0, 25 })));

    // Позиция не меньше самой себя.
    assert((!(Position{ 4, 2 } < Position{ 4, 2 })));

    std::cout << "TestPositionComparison OK\n";
}

void TestPositionToString() {
    assert((Position{ 0, 0 }.ToString() == "A1"));
    assert((Position{ 4, 2 }.ToString() == "C5"));

    assert((Position{ 0, 25 }.ToString() == "Z1"));
    assert((Position{ 0, 26 }.ToString() == "AA1"));
    assert((Position{ 0, 27 }.ToString() == "AB1"));

    assert((Position{ 0, 51 }.ToString() == "AZ1"));
    assert((Position{ 0, 52 }.ToString() == "BA1"));

    assert((Position{ 0, 701 }.ToString() == "ZZ1"));
    assert((Position{ 0, 702 }.ToString() == "AAA1"));

    const Position last{
        Position::MAX_ROWS - 1,
        Position::MAX_COLS - 1
    };

    assert(last.ToString() == "XFD16384");

    assert(Position::NONE.ToString().empty());
    assert((Position{ 0, Position::MAX_COLS }.ToString().empty()));

    std::cout << "TestPositionToString OK\n";
}

void TestPositionFromString() {
    assert((Position::FromString("A1") == Position{ 0, 0 }));
    assert((Position::FromString("C5") == Position{ 4, 2 }));

    assert((Position::FromString("Z1") == Position{ 0, 25 }));
    assert((Position::FromString("AA1") == Position{ 0, 26 }));
    assert((Position::FromString("AB1") == Position{ 0, 27 }));

    assert((Position::FromString("AZ1") == Position{ 0, 51 }));
    assert((Position::FromString("BA1") == Position{ 0, 52 }));

    assert((Position::FromString("ZZ1") == Position{ 0, 701 }));
    assert((Position::FromString("AAA1") == Position{ 0, 702 }));

    const Position last{
        Position::MAX_ROWS - 1,
        Position::MAX_COLS - 1
    };

    assert(Position::FromString("XFD16384") == last);

    assert(Position::FromString("") == Position::NONE);
    assert(Position::FromString("A") == Position::NONE);
    assert(Position::FromString("1") == Position::NONE);
    assert(Position::FromString("a1") == Position::NONE);

    assert(Position::FromString("A0") == Position::NONE);
    assert(Position::FromString("A-1") == Position::NONE);
    assert(Position::FromString("A+1") == Position::NONE);
    assert(Position::FromString("A1B") == Position::NONE);

    assert(Position::FromString(" A1") == Position::NONE);
    assert(Position::FromString("A1 ") == Position::NONE);

    assert(Position::FromString("XFE1") == Position::NONE);
    assert(Position::FromString("A16385") == Position::NONE);
    assert(Position::FromString("A99999999999999999999")
        == Position::NONE);

    std::cout << "TestPositionFromString OK\n";
}

void TestPositionRoundTrip() {
    const Position original{ 136, 702 };

    assert(Position::FromString(original.ToString()) == original);

    std::cout << "TestPositionRoundTrip OK\n";
}

void TestSize() {
    const Size empty;

    assert(empty.rows == 0);
    assert(empty.cols == 0);

    assert((Size{ 5, 3 } == Size{ 5, 3 }));
    assert((!(Size{ 5, 3 } == Size{ 3, 5 })));
    assert((!(Size{ 5, 3 } == Size{ 5, 4 })));
    assert((!(Size{ 5, 3 } == Size{ 6, 3 })));

    std::cout << "TestSize OK\n";
}

void TestFormulaError() {
    const FormulaError arithmetic{
        FormulaError::Category::Arithmetic
    };

    assert(arithmetic.GetCategory()
        == FormulaError::Category::Arithmetic);

    assert(arithmetic.ToString() == "#ARITHM!");

    const FormulaError same{
        FormulaError::Category::Arithmetic
    };

    const FormulaError value_error{
        FormulaError::Category::Value
    };

    const FormulaError reference_error{
        FormulaError::Category::Ref
    };

    assert(arithmetic == same);
    assert(!(arithmetic == value_error));

    assert(value_error.ToString() == "#VALUE!");
    assert(reference_error.ToString() == "#REF!");

    std::cout << "TestFormulaError OK\n";
}

void TestCellValue() {
    CellInterface::Value value = std::string{ "Hello" };

    assert(std::holds_alternative<std::string>(value));
    assert(std::get<std::string>(value) == "Hello");

    value = 5.0;

    assert(std::holds_alternative<double>(value));
    assert(std::get<double>(value) == 5.0);

    value = FormulaError{ FormulaError::Category::Arithmetic };

    assert(std::holds_alternative<FormulaError>(value));
    assert(std::get<FormulaError>(value).ToString() == "#ARITHM!");

    std::cout << "TestCellValue OK\n";
}

void TestEmptyCell() {
    auto sheet = CreateSheet();
    Cell cell(*sheet);

    assert(cell.GetText().empty());

    const auto value = cell.GetValue();

    assert(std::holds_alternative<std::string>(value));
    assert(std::get<std::string>(value).empty());

    assert(cell.GetReferencedCells().empty());

    std::cout << "TestEmptyCell OK\n";
}

void TestTextCell() {
    auto sheet = CreateSheet();
    Cell cell(*sheet);

    cell.Set("Hello");

    assert(cell.GetText() == "Hello");
    assert(std::get<std::string>(cell.GetValue()) == "Hello");

    cell.Set("World");

    assert(cell.GetText() == "World");
    assert(std::get<std::string>(cell.GetValue()) == "World");

    cell.Set("42");

    assert(cell.GetText() == "42");
    assert(std::holds_alternative<std::string>(cell.GetValue()));
    assert(std::get<std::string>(cell.GetValue()) == "42");

    assert(cell.GetReferencedCells().empty());

    std::cout << "TestTextCell OK\n";
}

void TestClearCell() {
    auto sheet = CreateSheet();
    Cell cell(*sheet);

    cell.Set("Hello");
    cell.Clear();

    assert(cell.GetText().empty());
    assert(std::get<std::string>(cell.GetValue()).empty());
    assert(cell.GetReferencedCells().empty());

    // Повторная очистка тоже допустима.
    cell.Clear();

    assert(cell.GetText().empty());

    // После очистки ячейкой можно пользоваться дальше.
    cell.Set("Again");

    assert(cell.GetText() == "Again");

    // Установка пустого текста.
    cell.Set("");

    assert(cell.GetText().empty());
    assert(std::get<std::string>(cell.GetValue()).empty());

    std::cout << "TestClearCell OK\n";
}

void TestCellThroughInterface() {
    auto sheet = CreateSheet();
    Cell cell(*sheet);
    cell.Set("Through interface");

    const CellInterface& interface = cell;

    assert(interface.GetText() == "Through interface");

    assert(std::get<std::string>(interface.GetValue())
        == "Through interface");

    assert(interface.GetReferencedCells().empty());

    std::cout << "TestCellThroughInterface OK\n";
}



void TestEscapedTextCell() {
    auto sheet = CreateSheet();
    Cell cell(*sheet);

    // Текст, внешне похожий на формулу.
    cell.Set("'=2+3");

    assert(cell.GetText() == "'=2+3");

    const auto value = cell.GetValue();

    assert(std::holds_alternative<std::string>(value));
    assert(std::get<std::string>(value) == "=2+3");
    assert(cell.GetReferencedCells().empty());

    // Чтение значения не меняет исходный текст.
    assert(cell.GetText() == "'=2+3");

    // Повторное чтение возвращает тот же результат.
    assert(std::get<std::string>(cell.GetValue()) == "=2+3");

    // Апостроф перед обычным текстом.
    cell.Set("'Hello");

    assert(cell.GetText() == "'Hello");
    assert(std::get<std::string>(cell.GetValue()) == "Hello");

    // Единственный апостроф даёт пустое значение.
    cell.Set("'");

    assert(cell.GetText() == "'");
    assert(std::get<std::string>(cell.GetValue()).empty());

    // Удаляется ровно один начальный апостроф.
    cell.Set("''Hello");

    assert(cell.GetText() == "''Hello");
    assert(std::get<std::string>(cell.GetValue()) == "'Hello");

    // Апостроф внутри текста сохраняется.
    cell.Set("It's text");

    assert(cell.GetText() == "It's text");
    assert(std::get<std::string>(cell.GetValue()) == "It's text");

    // Пробел перед апострофом: первый символ уже не апостроф.
    cell.Set(" 'Hello");

    assert(cell.GetText() == " 'Hello");
    assert(std::get<std::string>(cell.GetValue()) == " 'Hello");

    // Переход от экранированного текста к обычному.
    cell.Set("'Old");
    cell.Set("New");

    assert(cell.GetText() == "New");
    assert(std::get<std::string>(cell.GetValue()) == "New");

    // Очистка экранированного текста.
    cell.Set("'Text");
    cell.Clear();

    assert(cell.GetText().empty());
    assert(std::get<std::string>(cell.GetValue()).empty());

    std::cout << "TestEscapedTextCell OK\n";
}

void TestCellStateTransitions() {
    auto sheet = CreateSheet();
    Cell cell(*sheet);

    // Получаем интерфейс один раз.
    const CellInterface& interface = cell;

    // Пустая реализация.
    assert(interface.GetText().empty());
    assert(std::get<std::string>(interface.GetValue()).empty());

    // Переход к текстовой реализации.
    cell.Set("Hello");

    assert(interface.GetText() == "Hello");
    assert(std::get<std::string>(interface.GetValue()) == "Hello");

    // Замена текста экранированным текстом.
    cell.Set("'=A1");

    assert(interface.GetText() == "'=A1");
    assert(std::get<std::string>(interface.GetValue()) == "=A1");
    assert(interface.GetReferencedCells().empty());

    // Возврат к пустой реализации.
    cell.Clear();

    assert(interface.GetText().empty());
    assert(std::get<std::string>(interface.GetValue()).empty());

    // Снова текстовая реализация.
    cell.Set("Again");

    assert(interface.GetText() == "Again");

    // Установка пустого текста.
    cell.Set("");

    assert(interface.GetText().empty());
    assert(std::get<std::string>(interface.GetValue()).empty());

    std::cout << "TestCellStateTransitions OK\n";
}

void TestCellOwnershipThroughInterface() {
    auto sheet = CreateSheet();
    auto concrete_cell = std::make_unique<Cell>(*sheet);

    concrete_cell->Set("'Owned cell");

    std::unique_ptr<CellInterface> cell = std::move(concrete_cell);

    assert(concrete_cell == nullptr);
    assert(cell != nullptr);

    assert(cell->GetText() == "'Owned cell");
    assert(std::get<std::string>(cell->GetValue()) == "Owned cell");
    assert(cell->GetReferencedCells().empty());

    std::cout << "TestCellOwnershipThroughInterface OK\n";
}

void TestSheetStorage() {
    auto sheet = CreateSheet();

    const Position a1 = Position::FromString("A1");
    const Position c5 = Position::FromString("C5");

    assert(sheet->GetCell(a1) == nullptr);

    sheet->SetCell(a1, "Hello");
    sheet->SetCell(c5, "'=2+3");

    const CellInterface* first = sheet->GetCell(a1);

    assert(first != nullptr);
    assert(first->GetText() == "Hello");

    const CellInterface* second = sheet->GetCell(c5);

    assert(second != nullptr);
    assert(second->GetText() == "'=2+3");
    assert(std::get<std::string>(second->GetValue()) == "=2+3");

    // Изменение сохраняет объект существующей ячейки.
    sheet->SetCell(a1, "World");

    assert(sheet->GetCell(a1) == first);
    assert(first->GetText() == "World");

    // Проверяем константную версию GetCell().
    const SheetInterface& const_sheet = *sheet;

    assert(const_sheet.GetCell(a1) == first);
    assert(const_sheet.GetCell(a1)->GetText() == "World");

    std::cout << "TestSheetStorage OK\n";
}

void TestSheetEmptyAndClear() {
    auto sheet = CreateSheet();

    const Position a1 = Position::FromString("A1");

    sheet->SetCell(a1, "");

    assert(sheet->GetCell(a1) != nullptr);
    assert(sheet->GetCell(a1)->GetText().empty());

    sheet->SetCell(a1, "Hello");
    sheet->ClearCell(a1);

    assert(sheet->GetCell(a1) == nullptr);

    sheet->ClearCell(a1);

    assert(sheet->GetCell(a1) == nullptr);

    sheet->SetCell(a1, "Again");

    assert(sheet->GetCell(a1)->GetText() == "Again");

    std::cout << "TestSheetEmptyAndClear OK\n";
}

void TestSheetInvalidPosition() {
    auto sheet = CreateSheet();

    bool caught = false;

    try {
        sheet->SetCell(Position{ -1, 0 }, "Hello");
    }
    catch (const InvalidPositionException&) {
        caught = true;
    }

    assert(caught);

    caught = false;

    try {
        sheet->GetCell(Position{ 0, Position::MAX_COLS });
    }
    catch (const InvalidPositionException&) {
        caught = true;
    }

    assert(caught);

    caught = false;

    try {
        sheet->ClearCell(Position{ Position::MAX_ROWS, 0 });
    }
    catch (const InvalidPositionException&) {
        caught = true;
    }

    assert(caught);

    std::cout << "TestSheetInvalidPosition OK\n";
}
void ShowTableExample() {
    auto sheet = CreateSheet();

    sheet->SetCell(Position::FromString("A1"), "Name");
    sheet->SetCell(Position::FromString("B1"), "Value");

    sheet->SetCell(Position::FromString("A2"), "Text");
    sheet->SetCell(Position::FromString("B2"), "'=2+3");

    const Size size = sheet->GetPrintableSize();

    std::cout << "\nTable size: "
        << size.rows << " rows, "
        << size.cols << " columns\n";

    std::cout << "\nTexts:\n";
    sheet->PrintTexts(std::cout);

    std::cout << "\nValues:\n";
    sheet->PrintValues(std::cout);

    sheet->ClearCell(Position::FromString("B2"));

    const Size after_b2 = sheet->GetPrintableSize();

    std::cout << "\nAfter removing B2: "
        << after_b2.rows << " rows, "
        << after_b2.cols << " columns\n";

    sheet->PrintValues(std::cout);

    sheet->ClearCell(Position::FromString("B1"));

    const Size after_b1 = sheet->GetPrintableSize();

    std::cout << "\nAfter removing B1: "
        << after_b1.rows << " rows, "
        << after_b1.cols << " columns\n";

    sheet->PrintValues(std::cout);
}

void ShowEmptyCellExample() {
    auto sheet = CreateSheet();

    const Position c5 = Position::FromString("C5");

    // Создаём ячейку с пустым текстом.
    sheet->SetCell(c5, "");

    const Size empty_size = sheet->GetPrintableSize();

    std::cout << "\nEmpty text:\n";

    std::cout << "Cell exists: "
        << (sheet->GetCell(c5) != nullptr ? "yes" : "no")
        << '\n';

    std::cout << "Table size: "
        << empty_size.rows << " x "
        << empty_size.cols << '\n';

    // Сохраняем один апостроф.
    sheet->SetCell(c5, "'");

    const CellInterface* cell = sheet->GetCell(c5);
    const Size apostrophe_size = sheet->GetPrintableSize();

    std::cout << "\nSingle apostrophe:\n";

    std::cout << "Text: [" << cell->GetText() << "]\n";

    std::cout << "Value: ["
        << std::get<std::string>(cell->GetValue())
        << "]\n";

    std::cout << "Table size: "
        << apostrophe_size.rows << " x "
        << apostrophe_size.cols << '\n';

    // Удаляем саму ячейку.
    sheet->ClearCell(c5);

    const Size cleared_size = sheet->GetPrintableSize();

    std::cout << "\nAfter removal:\n";

    std::cout << "Cell exists: "
        << (sheet->GetCell(c5) != nullptr ? "yes" : "no")
        << '\n';

    std::cout << "Table size: "
        << cleared_size.rows << " x "
        << cleared_size.cols << '\n';
}

class FormulaTraceListener : public FormulaBaseListener {
public:
    void exitLiteral(FormulaParser::LiteralContext* ctx) override {
        std::cout << "Number: "
            << ctx->NUMBER()->getText()
            << '\n';
    }

    void exitBinaryOp(FormulaParser::BinaryOpContext* ctx) override {
        std::cout << "Operation: "
            << ctx->getText()
            << '\n';
    }
};

void ShowFormulaParse() {
    const std::string expression = "2+3*4";

    antlr4::ANTLRInputStream input(expression);
    FormulaLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    FormulaParser parser(&tokens);

    antlr4::tree::ParseTree* tree = parser.main();

    std::cout << "\nFormula: " << expression << '\n';
    std::cout << "Parse tree:\n";
    std::cout << tree->toStringTree(&parser) << '\n';

    FormulaTraceListener listener;

    std::cout << "Tree traversal:\n";

    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
}

void ShowNumberExpression() {
    auto sheet = CreateSheet();

    std::unique_ptr<ASTImpl::Expr> expression =
        std::make_unique<ASTImpl::NumberExpr>(42.0);

    std::cout << "\nNumber expression result: "
        << expression->Evaluate(*sheet)
        << '\n';
}

void ShowBinaryExpression() {
    auto sheet = CreateSheet();

    auto three = std::make_unique<ASTImpl::NumberExpr>(3.0);
    auto four = std::make_unique<ASTImpl::NumberExpr>(4.0);

    auto multiply = std::make_unique<ASTImpl::BinaryExpr>(ASTImpl::BinaryOperation::Multiply, std::move(three), std::move(four));

    auto two = std::make_unique<ASTImpl::NumberExpr>(2.0);

    auto addition = std::make_unique<ASTImpl::BinaryExpr>(ASTImpl::BinaryOperation::Add, std::move(two), std::move(multiply));

    const double result = addition->Evaluate(*sheet);

    std::cout << "\nBinary expression: 2 + 3 * 4\n";
    std::cout << "Result: " << result << '\n';
}


void ShowAutomaticExpression() {
    struct Example {
        std::string expression;
        double expected;
    };

    const std::vector<Example> examples = {
        {"2+3*4", 14.0},
        {"10-3", 7.0},
        {"20-5-3", 12.0},
        {"20-3*4", 8.0},
        {"20/4", 5.0},
        {"20/2/2", 5.0},
        {"10+12/3", 14.0},
        {"7/2", 3.5},
        {"(2+3)*4", 20.0},
        {"20-(5-3)", 18.0},
        {"20/(2*2)", 5.0},
        {"((7))", 7.0},
        {"-5", -5.0},
        {"+5", 5.0},
        {"-(2+3)", -5.0},
        {"2*-3", -6.0},
        {"--5", 5.0},
        {"10--3", 13.0}
    };

    auto sheet = CreateSheet();

    for (const auto& example : examples) {
        auto root = BuildExpression(example.expression);
        const double result = root->Evaluate(*sheet);

        std::cout << "\nAutomatic expression: " << example.expression << '\n';
        std::cout << "Result: " << result << '\n';

        assert(result == example.expected);
    }
}

void TestDivisionByZero() {
    auto root = BuildExpression("10/0");
    auto sheet = CreateSheet();

    bool error_caught = false;

    try {
        root->Evaluate(*sheet);
    }
    catch (const FormulaError& error) {
        assert(error.GetCategory() == FormulaError::Category::Arithmetic);
        error_caught = true;

        std::cout << "\nDivision by zero: caught\n";
    }

    assert(error_caught);
}

void TestFormulaSyntax() {
    const std::vector<std::string> valid_expressions = {
        "2+3*4",
        "-(2+3)",
        "10--3",
        " 2 + 3 "
    };

    for (const auto& expression : valid_expressions) {
        auto root = BuildExpression(expression);
        assert(root != nullptr);
    }

    const std::vector<std::string> invalid_expressions = {
        "",
        "2+",
        "3*/4",
        "(2+3",
        "2 3",
        "2+@3"
    };

    for (const auto& expression : invalid_expressions) {
        bool error_caught = false;

        try {
            BuildExpression(expression);
        }
        catch (const FormulaException&) {
            error_caught = true;

            std::cout << "Rejected formula: [" << expression << "]\n";
        }

        assert(error_caught);
    }

    std::cout << "TestFormulaSyntax OK\n";
}


void TestFormulaImplementation() {
    auto sheet = CreateSheet();

    auto formula = ParseFormula("2+3*4");

    assert(formula->GetExpression() == "2+3*4");
    assert(formula->GetReferencedCells().empty());

    const auto value = formula->Evaluate(*sheet);

    assert(std::holds_alternative<double>(value));
    assert(std::get<double>(value) == 14.0);

    const auto repeated_value = formula->Evaluate(*sheet);

    assert(std::holds_alternative<double>(repeated_value));
    assert(std::get<double>(repeated_value) == 14.0);

    auto division = ParseFormula("10/0");
    const auto error_value = division->Evaluate(*sheet);

    assert(std::holds_alternative<FormulaError>(error_value));

    const auto& error = std::get<FormulaError>(error_value);
    assert(error.GetCategory() == FormulaError::Category::Arithmetic);

    bool syntax_error_caught = false;

    try {
        auto invalid_formula = ParseFormula("2+");
    }
    catch (const FormulaException&) {
        syntax_error_caught = true;
    }

    assert(syntax_error_caught);

    std::cout << "TestFormulaImplementation OK\n";
}

void TestFormulaCell() {
    auto sheet = CreateSheet();
    const Position position = Position::FromString("A1");

    sheet->SetCell(position, "=2+3*4");

    const CellInterface* cell = sheet->GetCell(position);
    assert(cell != nullptr);
    assert(cell->GetText() == "=2+3*4");
    assert(std::get<double>(cell->GetValue()) == 14.0);

    bool syntax_error_caught = false;

    try {
        sheet->SetCell(position, "=2+");
    }
    catch (const FormulaException&) {
        syntax_error_caught = true;
    }

    assert(syntax_error_caught);

    cell = sheet->GetCell(position);
    assert(cell->GetText() == "=2+3*4");
    assert(std::get<double>(cell->GetValue()) == 14.0);

    sheet->SetCell(position, "=10/0");

    const auto error_value = sheet->GetCell(position)->GetValue();
    assert(std::holds_alternative<FormulaError>(error_value));

    const auto& error = std::get<FormulaError>(error_value);
    assert(error.GetCategory() == FormulaError::Category::Arithmetic);

    sheet->SetCell(position, "'=2+3");

    cell = sheet->GetCell(position);
    assert(cell->GetText() == "'=2+3");
    assert(std::get<std::string>(cell->GetValue()) == "=2+3");

    sheet->SetCell(position, "=");
    assert(std::get<std::string>(sheet->GetCell(position)->GetValue()) == "=");

    sheet->SetCell(position, "");
    assert(std::get<std::string>(sheet->GetCell(position)->GetValue()).empty());

    std::cout << "TestFormulaCell OK\n";
}

void TestCellReferences() {
    auto sheet = CreateSheet();

    const Position a1 = Position::FromString("A1");
    const Position b1 = Position::FromString("B1");
    const Position c1 = Position::FromString("C1");

    sheet->SetCell(a1, "10");
    sheet->SetCell(b1, "=A1*2");
    sheet->SetCell(c1, "=B1+5");

    assert(std::get<double>(sheet->GetCell(b1)->GetValue()) == 20.0);
    assert(std::get<double>(sheet->GetCell(c1)->GetValue()) == 25.0);

    sheet->SetCell(a1, "7");

    assert(std::get<double>(sheet->GetCell(b1)->GetValue()) == 14.0);
    assert(std::get<double>(sheet->GetCell(c1)->GetValue()) == 19.0);

    sheet->ClearCell(a1);

    assert(std::get<double>(sheet->GetCell(b1)->GetValue()) == 0.0);
    assert(std::get<double>(sheet->GetCell(c1)->GetValue()) == 5.0);

    sheet->SetCell(a1, "hello");

    const auto text_error = sheet->GetCell(c1)->GetValue();
    assert(std::holds_alternative<FormulaError>(text_error));
    assert(std::get<FormulaError>(text_error).GetCategory() == FormulaError::Category::Value);

    sheet->SetCell(a1, "=10/0");

    const auto arithmetic_error = sheet->GetCell(c1)->GetValue();
    assert(std::holds_alternative<FormulaError>(arithmetic_error));
    assert(std::get<FormulaError>(arithmetic_error).GetCategory() == FormulaError::Category::Arithmetic);

    sheet->SetCell(b1, "=A0");

    const auto reference_error = sheet->GetCell(b1)->GetValue();
    assert(std::holds_alternative<FormulaError>(reference_error));
    assert(std::get<FormulaError>(reference_error).GetCategory() == FormulaError::Category::Ref);

    std::cout << "TestCellReferences OK\n";
}

void TestReferencedCells() {
    const Position a1 = Position::FromString("A1");
    const Position b1 = Position::FromString("B1");
    const Position c2 = Position::FromString("C2");

    auto formula = ParseFormula("B1+A1+B1+(-C2)");

    const std::vector<Position> expected = { a1, b1, c2 };
    assert(formula->GetReferencedCells() == expected);

    auto arithmetic = ParseFormula("2+3*4");
    assert(arithmetic->GetReferencedCells().empty());

    auto invalid_reference = ParseFormula("A0+A1");
    const std::vector<Position> expected_valid = { a1 };
    assert(invalid_reference->GetReferencedCells() == expected_valid);

    auto sheet = CreateSheet();
    const Position d1 = Position::FromString("D1");

    sheet->SetCell(d1, "=B1+A1+B1+(-C2)");
    assert(sheet->GetCell(d1)->GetReferencedCells() == expected);

    sheet->SetCell(d1, "=A1");
    assert(sheet->GetCell(d1)->GetReferencedCells() == expected_valid);

    sheet->SetCell(d1, "Text");
    assert(sheet->GetCell(d1)->GetReferencedCells().empty());

    std::cout << "TestReferencedCells OK\n";
}

void TestCyclesAndCache() {
    class CountingSheet final : public Sheet {
    public:
        const CellInterface* GetCell(Position pos) const override {
            ++reads;
            return Sheet::GetCell(pos);
        }

        mutable int reads = 0;
    };

    CountingSheet sheet;

    const Position a1 = Position::FromString("A1");
    const Position b1 = Position::FromString("B1");
    const Position c1 = Position::FromString("C1");
    const Position d1 = Position::FromString("D1");

    sheet.SetCell(a1, "10");
    sheet.SetCell(b1, "=A1*2");
    sheet.SetCell(c1, "=B1+5");

    const CellInterface* result = sheet.GetCell(c1);

    // Первое вычисление читает другие ячейки.
    sheet.reads = 0;
    assert(std::get<double>(result->GetValue()) == 25.0);
    assert(sheet.reads > 0);

    // Повторное чтение использует кеш.
    sheet.reads = 0;
    assert(std::get<double>(result->GetValue()) == 25.0);
    assert(sheet.reads == 0);

    // Изменение посторонней ячейки сохраняет кеш.
    sheet.SetCell(d1, "Unrelated");

    sheet.reads = 0;
    assert(std::get<double>(result->GetValue()) == 25.0);
    assert(sheet.reads == 0);

    // Изменение исходного значения обновляет всю цепочку.
    sheet.SetCell(a1, "7");

    assert(result == sheet.GetCell(c1));
    assert(std::get<double>(result->GetValue()) == 19.0);

    // Косвенный цикл отклоняется, старое содержимое остаётся.
    bool caught = false;

    try {
        sheet.SetCell(a1, "=C1");
    }
    catch (const CircularDependencyException&) {
        caught = true;
    }

    assert(caught);
    assert(sheet.GetCell(a1)->GetText() == "7");
    assert(std::get<double>(result->GetValue()) == 19.0);

    // Прямая ссылка на себя тоже запрещена.
    const Position e1 = Position::FromString("E1");
    caught = false;

    try {
        sheet.SetCell(e1, "=E1+1");
    }
    catch (const CircularDependencyException&) {
        caught = true;
    }

    assert(caught);
    assert(sheet.GetCell(e1) == nullptr);

    // Удаление и повторное создание исходной ячейки.
    sheet.ClearCell(a1);
    assert(std::get<double>(result->GetValue()) == 5.0);

    sheet.SetCell(a1, "3");
    assert(std::get<double>(result->GetValue()) == 11.0);

    // Закешированная ошибка исчезает после исправления данных.
    sheet.SetCell(a1, "hello");
    assert(std::holds_alternative<FormulaError>(result->GetValue()));

    sheet.SetCell(a1, "4");
    assert(std::get<double>(result->GetValue()) == 13.0);

    // Замена формулы меняет её зависимости.
    sheet.SetCell(b1, "=D1");
    sheet.SetCell(d1, "8");
    assert(std::get<double>(result->GetValue()) == 13.0);

    // A1 больше не влияет на результат.
    sheet.SetCell(a1, "99");

    sheet.reads = 0;
    assert(std::get<double>(result->GetValue()) == 13.0);
    assert(sheet.reads == 0);

    // Удаление промежуточной ячейки и восстановление связи.
    sheet.ClearCell(b1);
    assert(std::get<double>(result->GetValue()) == 5.0);

    sheet.SetCell(b1, "=A1");
    assert(std::get<double>(result->GetValue()) == 104.0);

    std::cout << "TestCyclesAndCache OK\n";
}

void TestFinalCases() {
    const std::vector<std::pair<std::string, std::string>> examples = {
        {" ((2)) + (3 * 4) ", "2+3*4"},
        {"(2+3)*4", "(2+3)*4"},
        {"20-(5-3)", "20-(5-3)"},
        {"20/(2*2)", "20/(2*2)"},
        {"-(2+3)", "-(2+3)"},
        {"1e16+(-1e16+1)", "1e16+(-1e16+1)"},
        {" ((A0)) ", "A0"}
    };

    auto sheet = CreateSheet();

    for (const auto& [source, expected] : examples) {
        auto formula = ParseFormula(source);

        assert(formula->GetExpression() == expected);

        auto restored = ParseFormula(formula->GetExpression());

        assert(formula->Evaluate(*sheet) == restored->Evaluate(*sheet));
    }

    auto overflow = ParseFormula("1e308*10");
    const auto value = overflow->Evaluate(*sheet);

    assert(std::holds_alternative<FormulaError>(value));
    assert(std::get<FormulaError>(value).GetCategory() == FormulaError::Category::Arithmetic);

    bool caught = false;

    try {
        ParseFormula("1e9999");
    }
    catch (const FormulaException&) {
        caught = true;
    }

    assert(caught);

    const Position a1 = Position::FromString("A1");
    const Position b1 = Position::FromString("B1");

    sheet->SetCell(a1, "10");
    sheet->SetCell(b1, "= ((A1)) + 2 ");

    assert(sheet->GetCell(b1)->GetText() == "=A1+2");

    std::ostringstream texts;
    std::ostringstream values;

    sheet->PrintTexts(texts);
    sheet->PrintValues(values);

    assert(texts.str() == "10\t=A1+2\n");
    assert(values.str() == "10\t12\n");

    caught = false;

    try {
        sheet->SetCell(a1, "=2+");
    }
    catch (const FormulaException&) {
        caught = true;
    }

    assert(caught);
    assert(sheet->GetCell(a1)->GetText() == "10");
    assert(std::get<double>(sheet->GetCell(b1)->GetValue()) == 12.0);

    std::cout << "TestFinalCases OK\n";
}

int main() {
    TestPositionValidity();
    TestPositionComparison();
    TestPositionToString();
    TestPositionFromString();
    TestPositionRoundTrip();
    TestSize();
    TestFormulaError();
    TestCellValue();

    TestEmptyCell();
    TestTextCell();
    TestClearCell();
    TestCellThroughInterface();
    TestEscapedTextCell();
    TestCellStateTransitions();
    TestCellOwnershipThroughInterface();
    TestSheetStorage();
    TestSheetEmptyAndClear();
    TestSheetInvalidPosition();
    ShowTableExample();
    ShowEmptyCellExample();
    ShowFormulaParse();
    ShowNumberExpression();
    ShowBinaryExpression();
    ShowAutomaticExpression();
    TestDivisionByZero();
    TestFormulaSyntax();
    TestFormulaImplementation();
    TestFormulaCell();
    TestCellReferences();
    TestReferencedCells();
    TestCyclesAndCache();
    TestFinalCases();

    std::cout << "All tests passed.\n";

    return 0;
}