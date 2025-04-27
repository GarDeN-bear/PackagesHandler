/*
################################################################################
# Описание бинарного протокола
################################################################################

По сети ходят пакеты вида
packet : = size payload

size - размер последовательности, в количестве элементов, может быть 0.

payload - поток байтов(blob)
payload состоит из последовательности сериализованных переменных разных типов :

Описание типов и порядок их сериализации

type : = id(uint64_t) data(blob)

data : =
    IntegerType - uint64_t
    FloatType - double
    StringType - size(uint64_t) blob
    VectorType - size(uint64_t) ...(сериализованные переменные)

Все данные передаются в little endian порядке байтов

Необходимо реализовать сущности IntegerType, FloatType, StringType, VectorType
Кроме того, реализовать вспомогательную сущность Any
Сделать объект Serialisator с указанным интерфейсом.

Конструкторы(ы) типов IntegerType, FloatType и StringType должны обеспечивать
инициализацию, аналогичную инициализации типов uint64_t, double и std::string
соответственно. Конструктор(ы) типа VectorType должен позволять инициализировать
его списком любых из перечисленных типов(IntegerType, FloatType, StringType,
VectorType) переданных как по ссылке, так и по значению. Все указанные сигнатуры
должны быть реализованы. Сигнатуры шаблонных конструкторов условны, их можно в
конечной реализации делать на усмотрение разработчика, можно вообще убрать.
Vector::push должен быть именно шаблонной функцией. Принимает любой из типов:
IntegerType, FloatType, StringType, VectorType. Serialisator::push должен быть
именно шаблонной функцией.Принимает любой из типов: IntegerType, FloatType,
StringType, VectorType, Any Реализация всех шаблонных функций, должна
обеспечивать constraint requirements on template arguments, при этом,
использование static_assert - запрещается. Код в функции main не подлежит
изменению. Можно только добавлять дополнительные проверки.

Архитектурные требования :
1. Стаедарт - c++17
2. Запрещаются виртуальные функции.
3. Запрещается множественное и виртуальное наследование.
4. Запрещается создание каких - либо объектов в куче, в том числе с
использованием умных указателей. Это требование не влечет за собой огранечений
на использование std::vector, std::string и тп.
5. Запрещается любое дублирование кода, такие случаи должны быть строго
обоснованы. Максимально использовать обобщающие сущности. Например, если в
каждой из реализаций XType будет свой IdType getId() - это будет считаться
ошибкой.
6. Запрещается хранение value_type поля на уровне XType, оно должно быть
вынесено в обобщающую сущность.
7. Никаких других ограничений не накладывается, в том числе на создание
дополнительных обобщающих сущностей и хелперов.
8. XType должны реализовать сериализацию и десериализацию аналогичную Any.

Пример сериализации VectorType(StringType("qwerty"), IntegerType(100500))
{0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x71,0x77,0x65,0x72,0x74,0x79,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x94,0x88,
 0x01,0x00,0x00,0x00,0x00,0x00}
*/

#include <boost/type_index.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory.h>
#include <tuple>
#include <variant>
#include <vector>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

enum class TypeId : Id { Uint, Float, String, Vector };

namespace tools {

void printByte(const std::byte *byte) {
  std::cout << "\\0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(*byte) << ' ';
}

void printBuffer(const Buffer &buffer) {
  std::cout << "{ ";
  for (size_t i = 0; i < buffer.size(); ++i) {
    if (!(i % 8)) {
      std::cout << "\n ";
    }
    printByte(&buffer[i]);
  }
  std::cout << "\n}";
  std::cout << std::dec << std::endl;
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, uint64_t> ||
                                                  std::is_same_v<T, double> ||
                                                  std::is_same_v<T, char>>>
void serialize(Buffer &buffer, const T &value) {
  const std::byte *bytes = reinterpret_cast<const std::byte *>(&value);
  buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
  //   printBuffer(Buffer(buffer.end() - sizeof(T), buffer.end()));
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, uint64_t> ||
                                                  std::is_same_v<T, double> ||
                                                  std::is_same_v<T, char>>>
T deserialize(Buffer::const_iterator &_begin, Buffer::const_iterator _end) {
  if (std::distance(_begin, _end) < static_cast<std::ptrdiff_t>(sizeof(T))) {
    throw std::runtime_error("Not enough data to deserialize " +
                             boost::typeindex::type_id<T>().pretty_name());
  }

  T value;
  memcpy(&value, &*_begin, sizeof(T));
  _begin += sizeof(T);
  //   std::cout << boost::typeindex::type_id<T>().pretty_name() + ": " << value
  //             << std::endl;
  return value;
}

} // namespace tools

class IntegerType {

public:
  template <typename... Args, typename = std::enable_if_t<
                                  std::is_constructible_v<uint64_t, Args...>>>
  IntegerType(Args &&...args) : _value(std::forward<Args>(args)...) {}

  bool operator==(const IntegerType &_o) const { return _value == _o._value; }
  operator uint64_t() const { return _value; }

private:
  uint64_t _value;
};

class FloatType {

public:
  template <typename... Args, typename = std::enable_if_t<
                                  std::is_constructible_v<double, Args...>>>
  FloatType(Args &&...args) : _value(std::forward<Args>(args)...) {}

  bool operator==(const FloatType &_o) const { return _value == _o._value; }
  operator double() const { return _value; }

private:
  double _value;
};

class StringType {

public:
  template <typename... Args,
            typename =
                std::enable_if_t<std::is_constructible_v<std::string, Args...>>>
  StringType(Args &&...args) : _value(std::forward<Args>(args)...) {}

  bool operator==(const StringType &_o) const { return _value == _o._value; }
  operator std::string() const { return _value; }
  const char &operator[](size_t pos) const { return _value.at(pos); }
  char &operator[](size_t pos) { return _value.at(pos); }

private:
  std::string _value;
};

class Any;

class VectorType {
public:
  template <typename... Args, typename = std::enable_if_t<
                                  (std::is_constructible_v<Any, Args> && ...)>>
  VectorType(Args &&...args) {
    (push_back(std::forward<Args>(args)), ...);
  }

  template <typename Arg,
            typename = std::enable_if_t<std::is_constructible_v<Any, Arg>>>
  void push_back(Arg &&_val) {
    _value.emplace_back(std::forward<Arg>(_val));
  }

  size_t size() const { return _value.size(); }
  bool operator==(const VectorType &_o) const { return _value == _o._value; }
  operator std::vector<Any>() const { return _value; }
  const Any &operator[](size_t index) const { return _value.at(index); }
  Any &operator[](size_t index) { return _value.at(index); }

private:
  std::vector<Any> _value;
};

class Any {
public:
  Any() : _typeId(TypeId::Uint), _value(uint64_t{0}) {}
  template <typename... Args,
            typename = std::enable_if_t<(sizeof...(Args) >= 1)>>
  Any(Args &&...args) {
    if constexpr (sizeof...(Args) == 1) {
      auto &&first = std::get<0>(std::forward_as_tuple(args...));
      using DecayedType = std::decay_t<decltype(first)>;
      if constexpr (std::is_same_v<DecayedType, uint64_t>) {
        _typeId = TypeId::Uint;
        _value = std::forward<decltype(first)>(first);
      } else if constexpr (std::is_same_v<DecayedType, double>) {
        _typeId = TypeId::Float;
        _value = std::forward<decltype(first)>(first);
      } else if constexpr (std::is_same_v<DecayedType, std::string>) {
        _typeId = TypeId::String;
        _value = std::forward<decltype(first)>(first);
      } else if constexpr (std::is_same_v<DecayedType, std::vector<Any>>) {
        _typeId = TypeId::Vector;
        _value = std::forward<decltype(first)>(first);
      } else if constexpr (std::is_same_v<DecayedType, IntegerType>) {
        _typeId = TypeId::Uint;
        _value = static_cast<uint64_t>(first);
      } else if constexpr (std::is_same_v<DecayedType, FloatType>) {
        _typeId = TypeId::Float;
        _value = static_cast<double>(first);
      } else if constexpr (std::is_same_v<DecayedType, StringType>) {
        _typeId = TypeId::String;
        _value = static_cast<std::string>(first);
      } else if constexpr (std::is_same_v<DecayedType, VectorType>) {
        _typeId = TypeId::Vector;
        _value = static_cast<std::vector<Any>>(first);
      } else {
        _typeId = TypeId::Uint;
        _value = uint64_t{0};
      }
    } else {
      _typeId = TypeId::Vector;
      _value = std::vector<Any>{Any(std::forward<Args>(args))...};
    }
  }

  void serialize(Buffer &_buff) const {
    tools::serialize<uint64_t>(_buff, static_cast<Id>(_typeId));

    switch (_typeId) {
    case TypeId::Uint:
      tools::serialize<uint64_t>(_buff, std::get<uint64_t>(_value));
      break;
    case TypeId::Float:
      tools::serialize<double>(_buff, std::get<double>(_value));
      break;
    case TypeId::String: {
      const auto &value = std::get<std::string>(_value);
      tools::serialize<uint64_t>(_buff, value.size());
      for (const auto &item : value) {
        tools::serialize<char>(_buff, item);
      }
      break;
    }
    case TypeId::Vector: {
      const auto &value = std::get<std::vector<Any>>(_value);
      tools::serialize<uint64_t>(_buff, value.size());
      for (const auto &item : value) {
        item.serialize(_buff);
      }
      break;
    }
    }
  }

  Buffer::const_iterator deserialize(Buffer::const_iterator _begin,
                                     Buffer::const_iterator _end) {

    Id id = tools::deserialize<uint64_t>(_begin, _end);
    _typeId = static_cast<TypeId>(id);

    switch (_typeId) {
    case TypeId::Uint:
      _value = tools::deserialize<uint64_t>(_begin, _end);
      break;
    case TypeId::Float:
      _value = tools::deserialize<double>(_begin, _end);
      break;
    case TypeId::String: {
      const uint64_t size = tools::deserialize<uint64_t>(_begin, _end);
      std::string value;
      value.reserve(size);
      for (uint64_t i = 0; i < size; ++i) {
        value += tools::deserialize<char>(_begin, _end);
      }
      _value = value;
      //   std::cout << "string: ";
      //   for (unsigned char c : value) {
      //     std::cout << "\\0x" << std::hex << static_cast<int>(c);
      //   }
      //   std::cout << std::dec << std::endl;
      break;
    }
    case TypeId::Vector: {
      const uint64_t size = tools::deserialize<uint64_t>(_begin, _end);
      std::vector<Any> value;
      value.reserve(size);
      for (uint64_t i = 0; i < size; ++i) {
        Any item;
        _begin = item.deserialize(_begin, _end);
        value.push_back(item);
      }
      //   std::cout << "vector: " << value.size() << std::endl;
      _value = value;
      break;
    }
    }
    return _begin;
  }

  TypeId getPayloadTypeId() const { return _typeId; }

  template <typename Type> auto &getValue() const {
    return std::get<Type>(_value);
  }

  template <TypeId kId> auto &getValue() const {
    if constexpr (kId == TypeId::Uint) {
      return std::get<uint64_t>(_value);
    } else if constexpr (kId == TypeId::Float) {
      return std::get<double>(_value);
    } else if constexpr (kId == TypeId::String) {
      return std::get<std::string>(_value);
    } else if constexpr (kId == TypeId::Vector) {
      return std::get<std::vector<Any>>(_value);
    }
  }

  bool operator==(const Any &_o) const {
    return (_typeId == _o._typeId) && (_value == _o._value);
  }

private:
  std::variant<uint64_t, double, std::string, std::vector<Any>> _value;
  TypeId _typeId;
};

class Serializator {
public:
  template <typename Arg, typename = std::enable_if_t<std::disjunction_v<
                              std::is_same<std::decay_t<Arg>, IntegerType>,
                              std::is_same<std::decay_t<Arg>, FloatType>,
                              std::is_same<std::decay_t<Arg>, StringType>,
                              std::is_same<std::decay_t<Arg>, VectorType>,
                              std::is_same<std::decay_t<Arg>, Any>>>>
  void push(Arg &&_val) {
    storage_.push_back(std::forward<Arg>(_val));
  }

  Buffer serialize() const {
    Buffer buffer;
    tools::serialize<uint64_t>(buffer, storage_.size());

    for (const auto &item : storage_) {
      item.serialize(buffer);
    }

    return buffer;
  }

  static std::vector<Any> deserialize(const Buffer &_val) {
    Buffer::const_iterator it = _val.begin();
    const uint64_t size = tools::deserialize<uint64_t>(it, _val.end());
    std::vector<Any> result;
    result.reserve(size);

    for (uint64_t i = 0; i < size; ++i) {
      Any item;
      it = item.deserialize(it, _val.end());
      result.push_back(item);
    }

    return result;
  }

  const std::vector<Any> &getStorage() const { return storage_; }

private:
  std::vector<Any> storage_;
};

namespace tests {

Buffer getDeserializationTest() {
  return {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x03}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x02}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x02}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x06}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x71}, std::byte{0x77}, std::byte{0x65}, std::byte{0x72},
          std::byte{0x74}, std::byte{0x79}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}, std::byte{0x94}, std::byte{0x88},
          std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
          std::byte{0x00}, std::byte{0x00}};
}

VectorType getSerializationTest() {
  return VectorType(StringType("qwerty"), IntegerType(100500));
}

} // namespace tests

int main() {
  std::ifstream raw;
  raw.open("raw.bin", std::ios_base::in | std::ios_base::binary);
  if (!raw.is_open())
    return 1;
  raw.seekg(0, std::ios_base::end);
  std::streamsize size = raw.tellg();
  raw.seekg(0, std::ios_base::beg);
  Buffer buff(size);
  raw.read(reinterpret_cast<char *>(buff.data()), size);
  //   buff = tests::getDeserializationTest();
  auto res = Serializator::deserialize(buff);
  Serializator s;

  for (auto &&i : res)
    s.push(i);
  //   VectorType vec = tests::getSerializationTest();
  //   s.push(vec);
  //   buff = s.serialize();
  //   tools::printBuffer(buff);
  std::cout << (buff == s.serialize()) << '\n';
  return 0;
}
