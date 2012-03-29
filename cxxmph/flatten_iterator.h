#ifndef __CXXMPH_FLATTEN_ITERATOR_H__
#define __CXXMPH_FLATTEN_ITERATOR_H__

#include <cassert>
#include <vector>

namespace cxxmph {

template <typename container_type, typename outer_iterator_type, typename iterator_type>
struct flatten_iterator_base
    : public std::iterator<std::forward_iterator_tag,
                           typename container_type::value_type::value_type> {
  typedef container_type container;
  typedef outer_iterator_type outer_iterator;
  typedef iterator_type iterator;
  typedef flatten_iterator_base<container, outer_iterator, iterator>& self_reference;
  typedef typename iterator::reference reference;
  typedef typename iterator::pointer pointer;

  flatten_iterator_base(container* c, outer_iterator_type ot)
      : c_(c), ot_(ot), it_() { if (c_) skip_empty_outer(); }
  flatten_iterator_base(container* c, outer_iterator_type ot, iterator_type it)
      : c_(c), ot_(ot), it_(it) { }
  self_reference operator++() {
    ++it_;
    if (it_ == ot_->end()) skip_empty_outer();
    return *this;
  }
  reference operator*() { return *it_;  }
  pointer operator->() { return &(*it_); }

  // TODO find syntax to make this less permissible at compile time
  template <class T>
  bool operator==(const T& rhs) const { return rhs.ot_ == ot_ && rhs.it_ == this->it_; }
  template <class T>
  bool operator!=(const T& rhs) const { return !(rhs == *this); }

 public:  // TODO find syntax to make this friend of const iterator
  container* c_;
  outer_iterator_type ot_;
  iterator_type it_;
 private:
  void skip_empty_outer() {
    while (ot_ != c_->end() && (ot_->begin() == ot_->end() || it_ == ot_->end())) {
      ++ot_;
      it_ = iterator_type();
    }
    if (ot_ != c_->end() && ot_->begin() != ot_->end()) it_ = ot_->begin();
    else assert(it_ == iterator_type());
  }
};

template <typename container_type>
struct flatten_iterator : public flatten_iterator_base<
    container_type, typename container_type::iterator, typename container_type::value_type::iterator> {
  typedef flatten_iterator_base<
      container_type, typename container_type::iterator, typename container_type::value_type::iterator> parent_class;
  flatten_iterator() : parent_class(NULL, typename container_type::iterator())  { }
  flatten_iterator(typename parent_class::container* c,
                  typename parent_class::outer_iterator ot)
     : parent_class(c, ot) { }
  flatten_iterator(typename parent_class::container* c,
                   typename parent_class::outer_iterator ot,
                   typename parent_class::iterator it)
     : parent_class(c, ot, it) { }
};

template <typename container_type>
struct flatten_const_iterator : public flatten_iterator_base<
    const container_type, typename container_type::const_iterator, typename container_type::value_type::const_iterator> {
  typedef flatten_iterator_base<
      const container_type, typename container_type::const_iterator, typename container_type::value_type::const_iterator> parent_class;
  typedef flatten_const_iterator<container_type> self_type;
  typedef flatten_iterator<container_type> non_const_type;
  flatten_const_iterator(non_const_type rhs) : parent_class(rhs.c_, typename container_type::const_iterator(rhs.ot_), typename container_type::value_type::const_iterator(rhs.it_)) { }
  flatten_const_iterator() : parent_class(NULL, typename container_type::const_iterator())  { }
  flatten_const_iterator(typename parent_class::container* c,
                         typename parent_class::outer_iterator ot)
     : parent_class(c, ot) { }
  flatten_const_iterator(typename parent_class::container* c,
                         typename parent_class::outer_iterator ot,
                         typename parent_class::iterator it)
     : parent_class(c, ot, it) { }

};
template <typename container_type> flatten_iterator<container_type> make_flatten_begin(container_type* c) {
  return flatten_iterator<container_type>(c, c->begin());
}

template <typename container_type> flatten_const_iterator<container_type> make_flatten_begin(const container_type* c) {
  return flatten_const_iterator<container_type>(c, c->begin());
}

template <typename container_type> flatten_iterator<container_type> make_flatten_end(container_type* c) {
  return flatten_iterator<container_type>(c, c->end());
}

template <typename container_type> flatten_const_iterator<container_type> make_flatten_end(const container_type* c) {
  return flatten_const_iterator<container_type>(c, c->end());
}

template <typename container_type, typename outer_iterator_type, typename iterator_type>
flatten_iterator<container_type>
make_flatten(container_type* c, outer_iterator_type ot, iterator_type it) {
  return flatten_iterator<container_type>(c, ot, it);
}

template <typename container_type, typename outer_iterator_type, typename iterator_type>
flatten_const_iterator<container_type>
make_flatten(const container_type* c, outer_iterator_type ot, iterator_type it) {
  return flatten_const_iterator<container_type>(c, ot, it);
}
 

}  // namespace cxxmph

#endif  // __CXXMPH_FLATTEN_ITERATOR_H__
