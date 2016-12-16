/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XBROADCAST_HPP
#define XBROADCAST_HPP

#include <utility>
#include <initializer_list>
#include <vector>
#include <algorithm>

#include "xtensor/xutils.hpp"
#include "xtensor/xexpression.hpp"
#include "xtensor/xiterator.hpp"

namespace xt
{

    /*************
     * broadcast *
     *************/

    template <class E, class S>
    auto broadcast(E&& e, const S& s);

    template <class E>
    auto broadcast(E&& e, std::initializer_list<std::size_t> s);

    /**************
     * xbroadcast *
     **************/

    /**
     * @class xbroadcast
     * @brief Broadcasted xexpression to a specified shape.
     *
     * The xbroadcast class implements the broadcasting of an \ref xexpression
     * to a specified shape. xbroadcast is not meant to be used directly, but
     * only with the \ref broadcast helper functions.
     *
     * @tparam E the type of the \ref xexpression to broadcast
     * @tparam S the type of the specified shape.
     */
    template <class E, class X, bool LV>
    class xbroadcast : public xexpression<xbroadcast<E, X, LV>>
    {

    public:

        using self_type = xbroadcast<E, X, LV>;

        using value_type = typename E::value_type;
        using reference = typename E::reference;
        using const_reference = typename E::const_reference;
        using pointer = typename E::pointer;
        using const_pointer = typename E::const_pointer;
        using size_type = typename E::size_type;
        using difference_type = typename E::difference_type;
        
        using shape_type = promote_shape_t<typename E::shape_type, X>;
        using strides_type = promote_strides_t<typename E::strides_type, X>;
        using closure_type = const self_type;

        using const_stepper = typename E::const_stepper;
        using const_iterator = xiterator<const_stepper, shape_type>;
        using const_storage_iterator = const_iterator;

        template <class S>
        xbroadcast(const E& e, S s) noexcept;

        size_type dimension() const noexcept;
        const shape_type & shape() const noexcept;

        template <class... Args>
        const_reference operator()(Args... args) const;
        const_reference operator[](const xindex& index) const;

        template <class S>
        bool broadcast_shape(S& shape) const;

        template <class S>
        bool is_trivial_broadcast(const S& strides) const noexcept;

        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        template <class S>
        xiterator<const_stepper, S> xbegin(const S& shape) const noexcept;
        template <class S>
        xiterator<const_stepper, S> xend(const S& shape) const noexcept;
        template <class S>
        xiterator<const_stepper, S> cxbegin(const S& shape) const noexcept;
        template <class S>
        xiterator<const_stepper, S> cxend(const S& shape) const noexcept;

        template <class S>
        const_stepper stepper_begin(const S& shape) const noexcept;
        template <class S>
        const_stepper stepper_end(const S& shape) const noexcept;

        const_storage_iterator storage_begin() const noexcept;
        const_storage_iterator storage_end() const noexcept;

        const_storage_iterator storage_cbegin() const noexcept;
        const_storage_iterator storage_cend() const noexcept;

    private:

        std::conditional_t<LV, const E&, E> m_e;
        shape_type m_shape;
    };

    /****************************
     * broadcast implementation *
     ****************************/

    namespace detail
    {
        template <class R, class A>
        struct shape_forwarder
        {
            static inline R run(const A& r)
            {
                return R(r.cbegin(), r.cend());
            }
        };

        template <class R, class T>
        struct shape_forwarder<R, std::initializer_list<T>>
        {
            static inline R run(std::initializer_list<T> s)
            {
                return R(s);
            }
        };        

        template <class R>
        struct shape_forwarder<R, R>
        {
            static inline const R& run(const R& r)
            {
                return r;
            }
        };

        template <class R, class A>
        inline auto forward_shape(const A& s)
        {
            return shape_forwarder<R, A>::run(s);
        }
    }

    /**
     * @function broadcast
     * @brief Returns an \ref xexpression broadcasting the given expression to
     * a specified shape.
     *
     * @tparam e the \ref xexpression to broadcast
     * @tparam s the specified shape to broadcast.
     *
     * The returned expression either hold a const reference to \p e or a copy
     * depending on whether \p e is an lvalue or an rvalue.
     */
    template <class E, class S>
    inline auto broadcast(E&& e, const S& s)
    {
        constexpr bool is_lvalue = std::is_lvalue_reference<decltype(e)>::value;
        using broadcast_type = xbroadcast<get_xexpression_type<E>, S, is_lvalue>;
        using shape_type = typename broadcast_type::shape_type;
        return broadcast_type(std::forward<E>(e), detail::forward_shape<shape_type>(s));
    }

    template <class E>
    inline auto broadcast(E&& e, std::initializer_list<std::size_t> s)
    {
        // TODO: In the case of an initializer_list, use an array instead of a vector.
        using broadcast_type = xbroadcast<get_xexpression_type<E>, std::vector<std::size_t>, false>;
        using shape_type = typename broadcast_type::shape_type;
        return broadcast_type(std::forward<E>(e), detail::forward_shape<shape_type>(s));
    }

    /*****************************
     * xbroadcast implementation *
     *****************************/

    /**
     * @name Constructor
     */
    //@{
    /**
     * Constructs an xbroadcast expression broadcasting the specified
     * \ref xexpression to the given shape
     *
     * @param e the expression to broadcast
     * @param s the shape to apply
     */
    template <class E, class X, bool LV>
    template <class S>
    inline xbroadcast<E, X, LV>::xbroadcast(const E& e, S s) noexcept
        : m_e(e), m_shape(std::move(s))
    {
        xt::broadcast_shape(e.shape(), m_shape);
    }
    //@}


    /**
     * @name Size and shape
     */
    /**
     * Returns the number of dimensions of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::dimension() const noexcept -> size_type
    {
        return m_shape.size();
    }

    /**
     * Returns the shape of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::shape() const noexcept -> const shape_type &
    {
        return m_shape;
    }
    //@}

    /**
     * @name Data
     */
    /**
     * Returns a constant reference to the element at the specified position in the expression.
     * @param args a list of indices specifying the position in the function. Indices
     * must be unsigned integers, the number of indices should be equal or greater than
     * the number of dimensions of the expression.
     */
    template <class E, class X, bool LV>
    template <class... Args>
    inline auto xbroadcast<E, X, LV>::operator()(Args... args) const -> const_reference
    {
        return detail::get_element(m_e, args...);
    }

    /**
     * Compares the specified strides with those of the container to see whether
     * the broadcasting is trivial.
     * @return a boolean indicating whether the broadcasting is trivial
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::operator[](const xindex& index) const -> const_reference
    {
        // TODO:: depile
        return m_e[index];
    }
    //@}

    /**
     * @name Broadcasting
     */
    //@{
    /**
     * Broadcast the shape of the function to the specified parameter.
     * @param shape the result shape
     * @return a boolean indicating whether the broadcasting is trivial
     */
    template <class E, class X, bool LV>
    template <class S>
    inline bool xbroadcast<E, X, LV>::broadcast_shape(S& shape) const
    { 
        return xt::broadcast_shape(m_shape, shape);
    }

    /**
     * Compares the specified strides with those of the container to see whether
     * the broadcasting is trivial.
     * @return a boolean indicating whether the broadcasting is trivial
     */
    template <class E, class X, bool LV>
    template <class S>
    inline bool xbroadcast<E, X, LV>::is_trivial_broadcast(const S& strides) const noexcept
    {
        return dimension() == m_e.dimension() && std::equal(m_shape.cbegin(), m_shape.cend(), m_e.shape().cbegin()) &&
               m_e.is_trivial_broadcast(strides);
    }
    //@}

    /**
     * @name Iterators
     */
    //@{
    /**
     * Returns a constant iterator to the first element of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::begin() const noexcept -> const_iterator
    {
        return cxbegin(shape());
    }

    /**
     * Returns a constant iterator to the element following the last element
     * of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::end() const noexcept -> const_iterator
    {
        return cxend(shape());
    }

    /**
     * Returns a constant iterator to the first element of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::cbegin() const noexcept -> const_iterator
    {
        return cxbegin(shape());
    }

    /**
     * Returns a constant iterator to the element following the last element
     * of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::cend() const noexcept -> const_iterator
    {
        return cxend(shape());
    }

    /**
     * Returns a constant iterator to the first element of the expression. The
     * iteration is broadcasted to the specified shape.
     * @param shape the shape used for braodcasting
     */
    template <class E, class X, bool LV>
    template <class S>
    inline auto xbroadcast<E, X, LV>::xbegin(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        // Could check if (broadcastable(shape, m_shape)
        return xiterator<const_stepper, S>(stepper_begin(shape), shape);
    }

    /**
     * Returns a constant iterator to the element following the last element of the
     * expression. The iteration is broadcasted to the specified shape.
     * @param shape the shape used for broadcasting
     */
    template <class E, class X, bool LV>
    template <class S>
    inline auto xbroadcast<E, X, LV>::xend(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        // Could check if (broadcastable(shape, m_shape)
        return xiterator<const_stepper, S>(stepper_end(shape), shape);
    }

    /**
     * Returns a constant iterator to the first element of the expression. The
     * iteration is broadcasted to the specified shape.
     * @param shape the shape used for braodcasting
     */
    template <class E, class X, bool LV>
    template <class S>
    inline auto xbroadcast<E, X, LV>::cxbegin(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        // Could check if (broadcastable(shape, m_shape)
        return xiterator<const_stepper, S>(stepper_begin(shape), shape);
    }

    /**
     * Returns a constant iterator to the element following the last element of the
     * expression. The iteration is broadcasted to the specified shape.
     * @param shape the shape used for broadcasting
     */
    template <class E, class X, bool LV>
    template <class S>
    inline auto xbroadcast<E, X, LV>::cxend(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        // Could check if (broadcastable(shape, m_shape)
        return xiterator<const_stepper, S>(stepper_end(shape), shape);
    }
    //@}

    template <class E, class X, bool LV>
    template <class S>
    inline auto xbroadcast<E, X, LV>::stepper_begin(const S& shape) const noexcept -> const_stepper
    {
        // Could check if (broadcastable(shape, m_shape)
        return m_e.stepper_begin(shape);
    }

    template <class E, class X, bool LV>
    template <class S>
    inline auto xbroadcast<E, X, LV>::stepper_end(const S& shape) const noexcept -> const_stepper
    {
        // Could check if (broadcastable(shape, m_shape)
        return m_e.stepper_end(shape);
    }

    /**
     * @name Storage iterators
     */
    /**
     * Returns an iterator to the first element of the buffer
     * containing the elements of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::storage_begin() const noexcept -> const_storage_iterator
    {
        return cbegin();
    }

    /**
     * Returns an iterator to the element following the last
     * element of the buffer containing the elements of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::storage_end() const noexcept -> const_storage_iterator
    {
        return cend();
    }

    /**
     * Returns a constant iterator to the first element of the buffer
     * containing the elements of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::storage_cbegin() const noexcept -> const_storage_iterator
    {
        return cbegin();
    }

    /**
     * Returns a constant iterator to the element following the last
     * element of the buffer containing the elements of the expression.
     */
    template <class E, class X, bool LV>
    inline auto xbroadcast<E, X, LV>::storage_cend() const noexcept -> const_storage_iterator
    {
        return cend();
    }
    //@}

}

#endif
