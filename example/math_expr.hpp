#ifndef CTPEG_MATH_EXPR_HPP
#define CTPEG_MATH_EXPR_HPP
#include <cstdint>
#include <exception>

enum class ExprOp { PLUS, MINUS };
enum class FactorOp { TIMES, DIVIDE };

// Holds an operation (multiplication or division), and two operands for it.
struct Factor {
    int64_t lhs;
    FactorOp op;
    int64_t rhs;

    // Return the result of the stored operation
    [[nodiscard]] constexpr int64_t run() const noexcept {
        switch (op) {
            case FactorOp::TIMES:
                return lhs * rhs;
            case FactorOp::DIVIDE:
                return lhs / rhs;
        }
        // This shouldn't happen, will break compilation if reached at compile
        // time
        std::terminate();
    }
};

// Holds an operation (addition or subtraction), and two operands for it.
// Operands are factors
struct Expr {
    Factor lhs;
    ExprOp op;
    Factor rhs;

    // Return the result of the stored operation
    [[nodiscard]] constexpr int64_t run() const noexcept {
        switch (op) {
            case ExprOp::PLUS:
                return lhs.run() + rhs.run();
            case ExprOp::MINUS:
                return lhs.run() - rhs.run();
        }
        // This shouldn't happen, will break compilation if reached at compile
        // time
        std::terminate();
    }
};

#endif  // CTPEG_MATH_EXPR_HPP
