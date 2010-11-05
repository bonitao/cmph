#include "stringpiece.h"

namespace cxxmph {

template <typename iterator>
struct iterator_first : public iterator {
  iterator_first(iterator it) : iterator(it) { }
  const typename iterator::value_type::first_type& operator*() const {
    return this->iterator::operator*().first;
  }
};

template <typename iterator>
iterator_first<iterator> make_iterator_first(iterator it) {
  return iterator_first<iterator>(it);
}

template <typename value> class MakeStringPiece {
  public:
    StringPiece operator()(const value& v) { return StringPiece(reinterpret_cast<const char*>(&v), sizeof(value)); }
};
template <> class MakeStringPiece<std::string> {
  public:
    StringPiece operator()(const std::string& v) { return StringPiece(v); }
};
template <> class MakeStringPiece<const char*> {
  public:
    StringPiece operator()(const char* v) { return StringPiece(v); }
};

template <typename iterator>
struct iterator_stringpiece : public iterator {
  iterator_stringpiece(iterator it) : iterator(it) { }
  StringPiece operator*() const {
    return MakeStringPiece<typename iterator::value_type::first_type>()(this->iterator::operator*());
  }
};
template <typename iterator>
iterator_stringpiece<iterator> make_iterator_stringpiece(iterator it) {
  return iterator_stringpiece<iterator>(it);
}

}  // namespace cxxmph
