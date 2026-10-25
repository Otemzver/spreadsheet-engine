#include "formula.h"
#include "FormulaParserFacade.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace {

    class Formula final : public FormulaInterface {
    public:
        explicit Formula(std::string expression)
            : expression_(std::move(expression)), root_(BuildExpression(expression_)) {
            std::ostringstream output;
            root_->Print(output);
            expression_ = output.str();

            root_->CollectReferencedCells(referenced_cells_);

            std::sort(referenced_cells_.begin(), referenced_cells_.end());

            const auto unique_end = std::unique(referenced_cells_.begin(), referenced_cells_.end());
            referenced_cells_.erase(unique_end, referenced_cells_.end());
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return root_->Evaluate(sheet);
            }
            catch (const FormulaError& error) {
                return error;
            }
        }

        std::string GetExpression() const override {
            return expression_;
        }

        std::vector<Position> GetReferencedCells() const override {
            return referenced_cells_;
        }

    private:
        std::string expression_;
        std::unique_ptr<ASTImpl::Expr> root_;
        std::vector<Position> referenced_cells_;
    };

} // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}