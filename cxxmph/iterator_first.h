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
