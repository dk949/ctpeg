#ifndef CTPEG_MATH_EXPR_PARSER_HPP
#define CTPEG_MATH_EXPR_PARSER_HPP

#include <optional>
#include <stdexcept>
#include <string_view>
#include <variant>


// Adds user defined types to the ResultVariant before including the library
#define CTPEG_VARIANT Expr, Factor, ExprOp, FactorOp

#include "../ctpeg.hpp"

// A parser wrapped for converting characters '+' and '-' into their respective
// ExprOp
[[nodiscard]] constexpr ctpeg::Result ExprOpParser(
    std::string_view sv) noexcept {
    constexpr auto op = ctpeg::Choice(ctpeg::Char('+'), ctpeg::Char('-'));
    if (auto res = op(sv)) {
        ExprOp out;
        switch (std::get<char>(res.value().first)) {
            case '+':
                out = ExprOp::PLUS;
                break;
            case '-':
                out = ExprOp::MINUS;
                break;
        }
        return std::make_pair(out, res.value().second);
    } else {
        return tl::unexpected<ctpeg::Error_t>(res.error());
    }
}

// A parser wrapped for converting characters '*' and '/' into their respective
// FactorOp
[[nodiscard]] constexpr ctpeg::Result FactorOpParser(
    std::string_view sv) noexcept {
    constexpr auto op = ctpeg::Choice(ctpeg::Char('*'), ctpeg::Char('/'));
    if (auto res = op(sv)) {
        FactorOp out;
        switch (std::get<char>(res.value().first)) {
            case '*':
                out = FactorOp::TIMES;
                break;
            case '/':
                out = FactorOp::DIVIDE;
                break;
        }
        return std::make_pair(out, res.value().second);
    } else {
        return tl::unexpected<ctpeg::Error_t>(res.error());
    }
}

// A parser wrapped for converting the sequence of integers and FactorOp to
// Factor
[[nodiscard]] constexpr ctpeg::Result FactorParser(
    std::string_view sv) noexcept {
    constexpr auto factor = ctpeg::Choice(
        ctpeg::Sequence(
            ctpeg::Int(), ctpeg::Skip(ctpeg::Many(ctpeg::Char(' '))),
            FactorOpParser, ctpeg::Skip(ctpeg::Many(ctpeg::Char(' '))),
            ctpeg::Int()),
        ctpeg::Int());
    if (auto res = factor(sv)) {
        if (auto single = ctpeg::toVariant<ctpeg::ResultVariantSingle>(
                res.value().first)) {
            return std::make_pair(
                Factor{std::get<int64_t>(single.value()), FactorOp::TIMES, 1},
                res.value().second);
        } else {
            auto array = std::get<ctpeg::ResultVariantArray>(res.value().first);
            auto arr = array.cbegin();
            auto end = array.cend();

            arr = ctpeg::nextNonEmpty(arr, end);
            int64_t lhs = std::get<int64_t>(*arr);
            arr++;

            arr = ctpeg::nextNonEmpty(arr, end);
            FactorOp factorOp = std::get<FactorOp>(*arr);
            arr++;

            arr = ctpeg::nextNonEmpty(arr, end);
            int64_t rhs = std::get<int64_t>(*arr);

            return std::make_pair(Factor{lhs, factorOp, rhs},
                                  res.value().second);
        }
    } else {
        return tl::unexpected<ctpeg::Error_t>(res.error());
    }
}

// A parser wrapped for converting the sequence of Factors and ExprOps to
// Expression
[[nodiscard]] constexpr ctpeg::Result ExprParser(std::string_view sv) noexcept {
    constexpr auto expr = ctpeg::Choice(
        ctpeg::Sequence(
            FactorParser, ctpeg::Skip(ctpeg::Many(ctpeg::Char(' '))),
            ExprOpParser, ctpeg::Skip(ctpeg::Many(ctpeg::Char(' '))),
            FactorParser),
        FactorParser);
    if (auto res = expr(sv)) {
        if (auto single = ctpeg::toVariant<ctpeg::ResultVariantSingle>(
                res.value().first)) {
            return std::make_pair(
                Expr{std::get<Factor>(single.value()), ExprOp::PLUS,
                     Factor{0, FactorOp::TIMES, 0}},
                res.value().second);
        } else {
            auto array = std::get<ctpeg::ResultVariantArray>(res.value().first);
            auto arr = array.cbegin();
            auto end = array.cend();

            arr = ctpeg::nextNonEmpty(arr, end);
            auto lhs = std::get<Factor>(*arr);
            arr++;

            arr = ctpeg::nextNonEmpty(arr, end);
            ExprOp exprOp = std::get<ExprOp>(*arr);
            arr++;

            arr = ctpeg::nextNonEmpty(arr, end);
            auto rhs = std::get<Factor>(*arr);

            return std::make_pair(Expr{lhs, exprOp, rhs}, res.value().second);
        }
    } else {
        return tl::unexpected<ctpeg::Error_t>(res.error());
    }
}

constexpr auto parser = ctpeg::Final(ExprParser);

#endif  // CTPEG_MATH_EXPR_PARSER_HPP
