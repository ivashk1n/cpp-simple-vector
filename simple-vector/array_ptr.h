
#pragma once

#include <array>
#include <utility>

using namespace std;

//Умный указатель с move - семантикой, управляющий динамическим массивом объектов типа Type.
//(или сырой указатель в "умном" классе обёртке)
template <typename Type>
class ArrayPtr {
public:
    ArrayPtr() = default;

    // Конструктор задания массива array
    explicit ArrayPtr(size_t size)
        : raw_ptr_(size ? new Type[size]() : nullptr) {
    }

    ~ArrayPtr() {
        delete[] raw_ptr_;
    }

    // Удаляем конструктор копирования для использования move - семантики (запрещаем копирование)
    ArrayPtr(const ArrayPtr& other) = delete;

    // Удаляем оператор присваивания -=- (запрещаем копирование)
    ArrayPtr& operator=(const ArrayPtr& other) = delete;

    // Конструктор перемещения. && - rvalue ссылка (ссылка на временный объект). noexcept - не выбрасывает исключений
    // Можно использовать exchange, но без него яснее, далее будет использваться exchange
    ArrayPtr(ArrayPtr&& other) noexcept
        : raw_ptr_(other.raw_ptr_) {
        other.raw_ptr_ = nullptr;  // Забираем владение, старый объект теперь пуст
    }

    // Оператор перемещения. && - rvalue сылка
    ArrayPtr& operator=(ArrayPtr&& other) noexcept {
        if (this != &other) { // Случай с самоприсваиванием.
            delete[] raw_ptr_;      // Освобождаем старую память
            // можно использовать raw_ptr_ = exchange(other.raw_ptr_, nullptr); но без exchange яснее
            raw_ptr_ = other.raw_ptr_; // Забираем ресурс
            other.raw_ptr_ = nullptr;  // Старый объект теперь пуст

        }
        return *this;
    }

    Type* Get() const {
        return raw_ptr_;
    }

    Type& operator[](size_t index) {
        return raw_ptr_[index];
    }

    const Type& operator[](size_t index) const {
        return raw_ptr_[index];
    }

    void swap(ArrayPtr& other) noexcept {
        std::swap(raw_ptr_, other.raw_ptr_);
    }

private:
    Type* raw_ptr_ = nullptr;
};

