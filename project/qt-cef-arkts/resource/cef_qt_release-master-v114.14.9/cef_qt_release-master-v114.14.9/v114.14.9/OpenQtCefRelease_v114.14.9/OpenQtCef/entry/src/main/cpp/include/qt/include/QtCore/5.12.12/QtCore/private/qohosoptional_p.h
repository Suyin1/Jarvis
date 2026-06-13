#ifndef QOHOSOPTIONAL_H
#define QOHOSOPTIONAL_H

#include <QtCore/QtGlobal>
#include <stdexcept>
#include <type_traits>

QT_BEGIN_NAMESPACE

template<typename T>
class QOhosOptional
{
    static_assert(
        std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
        "Only copyable types are supported");

public:
    explicit QOhosOptional(const T &value);

    QOhosOptional() = default;
    QOhosOptional(const QOhosOptional<T> &other);
    QOhosOptional<T> &operator=(const QOhosOptional<T> &other);

    ~QOhosOptional();

    bool hasValue() const;
    T valueOr(const T &fallback) const;

    const T &value() const;
    T &value();

private:
    void reset();
    T &storedValueRef();
    const T &storedValueRef() const;
    template<typename TT>
    void initializeStoredValue(TT &&initValue);

    std::aligned_storage_t<sizeof(T), alignof(T)> m_rawStoredValue;
    bool m_hasValue = false;
};

template<typename T>
QOhosOptional<T>::QOhosOptional(const T &value)
{
    initializeStoredValue(value);
    m_hasValue = true;
}

template<typename T>
QOhosOptional<T>::QOhosOptional(const QOhosOptional<T> &other)
{
    if (other.m_hasValue) {
        initializeStoredValue(other.storedValueRef());
        m_hasValue = true;
    }
}

template<typename T>
QOhosOptional<T> &QOhosOptional<T>::operator=(const QOhosOptional<T> &other)
{
    if (&other == this) {
        return *this;
    }

    if (other.m_hasValue) {
        if (m_hasValue) {
            storedValueRef() = other.storedValueRef();
        } else {
            initializeStoredValue(other.storedValueRef());
            m_hasValue = true;
        }
    } else {
        reset();
    }

    return *this;
}

template<typename T>
QOhosOptional<T>::~QOhosOptional()
{
    reset();
}

template<typename T>
void QOhosOptional<T>::reset()
{
    if (m_hasValue) {
        m_hasValue = false;
        storedValueRef().~T();
    }
}

template<typename T>
T &QOhosOptional<T>::storedValueRef()
{
    return *reinterpret_cast<T *>(&m_rawStoredValue);
}

template<typename T>
const T &QOhosOptional<T>::storedValueRef() const
{
    return *reinterpret_cast<const T *>(&m_rawStoredValue);
}

template<typename T>
template<typename TT>
void QOhosOptional<T>::initializeStoredValue(TT &&initValue)
{
    new (&m_rawStoredValue) T(std::forward<TT>(initValue));
}

template<typename T>
bool QOhosOptional<T>::hasValue() const
{
    return m_hasValue;
}

template<typename T>
T QOhosOptional<T>::valueOr(const T &fallback) const
{
    return hasValue() ? storedValueRef() : fallback;
}

template<typename T>
const T &QOhosOptional<T>::value() const
{
    if (!m_hasValue) {
        throw std::runtime_error("Can't access value inside empty QOhosOptional<>");
    }

    return storedValueRef();
}

template<typename T>
T &QOhosOptional<T>::value()
{
    if (!m_hasValue) {
        throw std::runtime_error("Can't access value inside empty QOhosOptional<>");
    }

    return storedValueRef();
}

QT_END_NAMESPACE

#endif
