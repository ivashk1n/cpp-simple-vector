
#pragma once

#include "array_ptr.h"

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <iostream>
#include <array>
#include <stdexcept>
#include <utility>
#include <iterator>

using namespace std;
// Недостаток подключения using namespace std; в заголовычных файлах это конфликт имён
// Если в другом месте тоже есть cout, list, map, и т.п., они могут конфликтовать с std::cout, std::list, std::map.
// Понял недостаток, в дальнейщем не буду использвать using namespace std; в .h файлах

// Класс обёртка для различения конструкторов. (т.к оба конструктора имееют аргумент size_t что делает их нераличаемыми)
class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity) : capacity_to_reserve_(capacity) {}

    size_t GetCapacity() const {
        return capacity_to_reserve_;
    }

private:
    size_t capacity_to_reserve_;
};

// Функция принимает capacity_to_reserve и создаёт объект ReserveProxyObj, который передаётся в конструктор SimpleVector.
// например SimpleVector<int> v(Reserve(5)) 
//Вызов Reserve(5) создаёт объект ReserveProxyObj(5) и вызывает конструктор резервирования где меняется capacity_
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
};

// Упрощённый аналог контейнера vector на основе умного указателя
// Не использует класс vector.
template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Конструктор создаёт массив из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : storage_(size), size_(size), capacity_(size) {
        fill(storage_.Get(), storage_.Get() + size, Type{});
        // в теле конструктора необходима инициализация?  Ведь при storage_(size) вызавается конструктор умного указателя
        // в котором уже выполняется инициализация значениями по умолчанию
    }

    // Конструктор создаёт массив из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : storage_(size), size_(size), capacity_(size) {
        fill(storage_.Get(), storage_.Get() + size, value);
    }

    // Конструктор создаёт массив из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : storage_(init.size()), size_(init.size()), capacity_(init.size()) {
        copy(init.begin(), init.end(), storage_.Get());
    }

    // Конструктор копирования
    SimpleVector(const SimpleVector& other) : storage_(other.capacity_), size_(other.size_), capacity_(other.capacity_) {
        copy(other.storage_.Get(), other.storage_.Get() + other.size_, storage_.Get());
    }

    // Конструктор с резервированием памяти (выделяет capacity_ и память для неё)
    explicit SimpleVector(ReserveProxyObj proxy)
        : storage_(proxy.GetCapacity()), size_(0), capacity_(proxy.GetCapacity()) {
    }

    // Конструктор перемещения &&
    SimpleVector(SimpleVector&& other) noexcept
        : storage_(move(other.storage_)),
        size_(exchange(other.size_, 0)),
        capacity_(exchange(other.capacity_, 0)) {
    }

    // Обменивает значение с другим массивом
    void swap(SimpleVector& other) noexcept {
        storage_.swap(other.storage_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Оператор присваевания на основе copy and swap
    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) { // случай с самоприсваиванием
            if (rhs.GetSize() == 0) { // случай с присваеванием пустого контейнера
                storage_ = ArrayPtr<Type>{}; // пустой контейнер
                size_ = 0;
                capacity_ = 0;
            }
            else {
                SimpleVector temp(rhs); //копия 
                this->swap(temp); //swap() вызывается явно для this, изменяя текущий объект. 
            }
        return *this;
    }

    // Оператор присваевания && с move семантикой
    SimpleVector& operator=(SimpleVector&& other) noexcept {
        if (this != &other) {
            storage_ = move(other.storage_);
            size_ = exchange(other.size_, 0);
            capacity_ = exchange(other.capacity_, 0);
        }
        return *this;
    }

    // Деструктор классу SimpleVector не нужен. Объекты разрущает умный указатель
    //~SimpleVector() {
    //     delete[] storage_.Get();
    // }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw out_of_range("Ошибка:  index >= size_");
        }
        else {
            return storage_[index];
        }
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw out_of_range("Ошибка:  index >= size_");
        }
        else {
            return storage_[index];
        }
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }
  
    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            Reserve(new_size); // Выделение вместимости массива 
        }
        if (new_size > size_) {
            for (size_t i = size_; i < new_size; ++i) {
                storage_[i] = Type(); // Заполняем массив элементами по умолчанию ()
            }
        }
        size_ = new_size;
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return storage_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return storage_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return storage_.Get() + size_;
    }
   
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ >= capacity_) {
            Reserve(capacity_ == 0 ? 1 : 2 * capacity_); // Выделение вместимости
        }
        storage_[size_] = item;
        ++size_;
    }

    // Добавляет элемент по rvalue ссылке в конец вектора 
    void PushBack(Type&& item) {
        if (size_ >= capacity_) {
            Reserve(capacity_ == 0 ? 1 : 2 * capacity_);
        }
        storage_[size_] = move(item);
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        return Insert(pos, Type(value));
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end()); // Проверка на попадание в диапазон массива
        //size_t index = pos - begin(); // Индекс на вставленное значение и перевод его в size_t 
        // Не понял коммит к предыдущей строке, записал её иначе
        size_t index = distance(begin(), pos);

        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            Reserve(new_capacity); // Выделение вместимости   
        }
        move_backward(storage_.Get() + index, storage_.Get() + size_, storage_.Get() + size_ + 1);
        storage_[index] = move(value)
        ++size_;

        return begin() + index;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_ != 0) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    // и возвращает итератор, который ссылается на элемент, следующий за удалённым
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos <= end()); // Проверка на попадание в диапазон массива
        Iterator after_erase = storage_.Get() + (pos - storage_.Get()); // В скобках итератор удаляемого элемента и перевод его в size_t 
        move(after_erase + 1, storage_.Get() + size_, after_erase); // Сдвигаем элементы влево
        --size_;
        return after_erase;
    }

    // Резервирует вместимость. Удобно при знание кол-ва элементов заранее 
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_storage(new_capacity);
            move(begin(), end(), new_storage.Get());
            storage_ = move(new_storage);
            capacity_ = new_capacity;
        }
    }

private:
    //Type* storage_ = nullptr; // адрес начала массива (это сырой указатель он требует явного 
    //освобождения памяти (delete[]), а также увеличивает вероятность утечек памяти.
    //т.е автоматически не освобождает память

    ArrayPtr<Type> storage_; // Умный указатель который убирает недостатки сырого указателя

    size_t size_ = 0;
    size_t capacity_ = 0;
};



template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs.GetSize() == rhs.GetSize()) && equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs.GetSize() == rhs.GetSize()) || !(equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()));
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}