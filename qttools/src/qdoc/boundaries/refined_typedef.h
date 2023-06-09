// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qglobal.h>

#include <functional>
#include <optional>
#include <type_traits>

// TODO: Express the documentation such that QDoc would be able to see
// it and process it correctly. This probably means that we would like
// to associate the definition with a namespace, albeit we could use
// the header file too, and put the documentation in an empty cpp
// file. This is delayed as there currently isn't much namespacing for
// anything in QDoc and such a namespacing should be added gradually
// and attentively.

// TODO: Review the semantics for construction and optmize it. Should we copy the
// value? Should we only allow rvalues?

// TODO: There is an high chance that we will need to "compose"
// refinitions later on when dealing, for example, with the basics of
// user-provided paths.
// For example, when requiring that each user-inputted path is purged
// as per QFileInfo definition of purging.
// For example, it might be that instead of passing QString around we
// might pass some Path type that is a purged QString.
// Then, any other refinement over paths will need to use that as a
// base type.
// To avoid the clutter that comes from that, if such will be the
// case, we will need to change the definition of refine and value if
// the passed in type was refined already.
// That is, such that if we have:
//
// QDOC_REFINE_TYPE(QString, Path) { ... }
// QDOC_REFINE_TYPE(Path, Foo) { ... }
//
// Foo refines a QString and Foo.value returns a QString. This should
// in general be trivial as long as we add a way to identify, such as
// a tag that refinements derive from, what type was declared through
// QDOC_REFINED_TYPEDEF and what type was not.

// TODO: Provide a way to generate a standard documentation for all
// members of a type generated by QDOC_REFINED_TYPEDEF without having
// to copy-paste include command everywhere.
// The main problem of doing this is that the preprocessor strips
// comments away, making it impossible to generate comments, and hence
// QDoc documentation, with the preprocessor.

/*!
 * \macro QDOC_REFINED_TYPEDEF(_type, _name)
 * \relates refined_typedef.hpp
 *
 * Declares a wrapper type for \c {_type}, with identifier \c {_name},
 * that represents a subset of \c {_type} for which some conditions
 * hold.
 *
 * For example:
 *
 * \code
      QDOC_REFINED_TYPEDEF(std::size_t, Fin5) {
          return (value < 5) : std::make_optional<Fin5>{value} : std::nullopt;
      }
 * \endcode
 *
 * Represents the subset of \c {std::size_t} that contains the value 0, 1,
 * 2, 3 and 4, that is, the general finite set of cardinality 5.
 *
 * As the example shows, usages of the macro require some type, an
 * identifier and some code.
 *
 * The type that is provided is the type that will be wrapped.
 * Do note that we expect a type with no-qualifiers and that is not a
 * pointer type. Types passed with those kind of qualifiers will be
 * simplified to their base type.
 *
 * That is, for example, \c {int*}, \c {const int}, \c {const int&},
 * \c {int&} all counts as \c {int}.
 *
 * The identifier that is passed is used as the name for the newly
 * declared type that wraps the original type.
 *
 * The code block that is passed will be run when an instance of the
 * newly created wrapper type is being obtained.
 * If the wrapper type is T, the codeblock must return a \c
 * {std::optional<T>}.
 * The code block should perform any check that ensures that the
 * guarantees provided by the wrapper type holds and return a value if
 * such is the case, otherwise returning \c {std::nullopt}.
 *
 * Inside the code block, the identifier \e {value} is implicitly
 * bound to an element of the wrapped type that the instance is being
 * built from and for which the guarantees provided by the wrapper
 * type must hold.
 *
 * When a call to QDOC_REFINED_TYPEDEF is successful, a type with the
 * provided identifier is declared.
 *
 * Let T be a type declared trough a call of QDOC_REFINED_TYPEDEF and
 * W be the type that it wraps.
 *
 * An instance of T can be obtained by calling T::refine with an
 * element of W.
 *
 * If the element of W respects the guarantees that T provides, then
 * the call will return an optional that contains an instance of T,
 * othewise it will return an empty optional.
 *
 * When an instance of T is obtained, it will wrap the element of W that
 * was used to obtain it.
 *
 * The wrapped value can be accessed trough the \c {value} method.
 *
 * For example, considering \c {Fin5}, we could obtain an instance of
 * it as follows:
 *
 * \code
 * auto instance = *(Fin5::refine(std::size_t{1}));
 * \endcode
 *
 * With that instance available we can retrieve the original value as
 * follows:
 *
 * \code
 * instance.value(); // The value 1
 * \endcode
 */

#define QDOC_REFINED_TYPEDEF(_type, _name)                                                                                                           \
    struct _name {                                                                                                                                   \
    public:                                                                                                                                          \
        using wrapped_type = std::remove_reference_t<std::remove_cv_t<std::remove_pointer_t<_type>>>;                                                \
                                                                                                                                                     \
        inline static constexpr auto has_equality_operator_v = std::is_invocable_r_v<bool, std::equal_to<>, wrapped_type, wrapped_type>;             \
        inline static constexpr auto has_less_than_operator_v = std::is_invocable_r_v<bool, std::less_equal<>, wrapped_type, wrapped_type>;          \
        inline static constexpr auto has_strictly_less_than_operator_v = std::is_invocable_r_v<bool, std::less<>, wrapped_type, wrapped_type>;       \
        inline static constexpr auto has_greater_than_operator_v = std::is_invocable_r_v<bool, std::greater_equal<>, wrapped_type, wrapped_type>;    \
        inline static constexpr auto has_strictly_greater_than_operator_v = std::is_invocable_r_v<bool, std::greater<>, wrapped_type, wrapped_type>; \
                                                                                                                                                     \
    public:                                                                                                                                          \
        static std::optional<_name> refine(wrapped_type value);                                                                                      \
                                                                                                                                                     \
        [[nodiscard]] const wrapped_type& value() const noexcept { return _value; }                                                                  \
                                                                                                                                                     \
        _name(const _name&) = default;                                                                                                               \
        _name& operator=(const _name&) = default;                                                                                                    \
                                                                                                                                                     \
        _name(_name&&) = default;                                                                                                                    \
        _name& operator=(_name&&) = default;                                                                                                         \
                                                                                                                                                     \
        operator wrapped_type() const { return _value; }                                                                                             \
                                                                                                                                                     \
    public:                                                                                                                                          \
                                                                                                                                                     \
        template<                                                                                                                                    \
            typename = std::enable_if_t<                                                                                                             \
                has_equality_operator_v                                                                                                              \
            >                                                                                                                                        \
        >                                                                                                                                            \
        bool operator==(const _name& rhs) const noexcept { return _value == rhs._value; }                                                            \
                                                                                                                                                     \
        template<                                                                                                                                    \
            typename = std::enable_if_t<                                                                                                             \
                has_equality_operator_v                                                                                                              \
            >                                                                                                                                        \
        >                                                                                                                                            \
        bool operator!=(const _name& rhs) const noexcept { return !(_value == rhs._value); }                                                         \
                                                                                                                                                     \
        template<                                                                                                                                    \
            typename = std::enable_if_t<                                                                                                             \
                has_less_than_operator_v                                                                                                             \
            >                                                                                                                                        \
        >                                                                                                                                            \
        bool operator<=(const _name& rhs) const noexcept { return _value <= rhs._value; }                                                            \
                                                                                                                                                     \
                                                                                                                                                     \
        template<                                                                                                                                    \
            typename = std::enable_if_t<                                                                                                             \
                has_strictly_less_than_operator_v                                                                                                    \
            >                                                                                                                                        \
        >                                                                                                                                            \
        bool operator<(const _name& rhs) const noexcept { return _value < rhs._value; }                                                              \
                                                                                                                                                     \
        template<                                                                                                                                    \
            typename = std::enable_if_t<                                                                                                             \
                has_greater_than_operator_v                                                                                                          \
            >                                                                                                                                        \
        >                                                                                                                                            \
        bool operator>=(const _name& rhs) const noexcept { return _value >= rhs._value; }                                                            \
                                                                                                                                                     \
        template<                                                                                                                                    \
            typename = std::enable_if_t<                                                                                                             \
                has_strictly_greater_than_operator_v                                                                                                 \
            >                                                                                                                                        \
        >                                                                                                                                            \
        bool operator>(const _name& rhs) const noexcept { return _value > rhs._value; }                                                              \
                                                                                                                                                     \
    private:                                                                                                                                         \
        _name(wrapped_type value) : _value{std::move(value)} {}                                                                                      \
                                                                                                                                                     \
    private:                                                                                                                                         \
        wrapped_type _value;                                                                                                                         \
    };                                                                                                                                               \
                                                                                                                                                     \
    inline std::optional<_name> _name::refine(wrapped_type value)
