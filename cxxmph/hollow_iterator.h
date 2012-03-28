#ifndef __CXXMPH_HOLLOW_ITERATOR_H__
#define __CXXMPH_HOLLOW_ITERATOR_H__

#include <vector>

namespace cxxmph {

template <typename container_type, typename presence_type, typename iterator_type>
struct hollow_iterator_base
    : public std::iterator<std::forward_iterator_tag,
                           typename container_type::value_type> {
  typedef presence_type presence;
  typedef container_type container;
  typedef iterator_type iterator;
  typedef hollow_iterator_base<container, presence, iterator>& self_reference;
  typedef typename iterator::reference reference;
  typedef typename iterator::pointer pointer;

  hollow_iterator_base(container* c, presence* p, iterator it)
      : c_(c), p_(p), it_(it) { if (c_) find_present(); }
  self_reference operator++() {
    ++it_; find_present();
  }
  reference operator*() { return *it_;  }
  pointer operator->() { return &(*it_); }

  // TODO find syntax to make this less permissible at compile time
  template <class T>
  bool operator==(const T& rhs) const { return rhs.it_ == this->it_; }
  template <class T>
  bool operator!=(const T& rhs) const { return rhs.it_ != this->it_; }

 public:  // TODO find syntax to make this friend of const iterator
  void find_present() {
    while (it_ != c_->end() && !((*p_)[it_-c_->begin()])) ++it_;
  }
  container* c_;
  presence* p_;
  iterator it_;
};

template <typename container_type>
struct hollow_iterator : public hollow_iterator_base<
    container_type, std::vector<bool>, typename container_type::iterator> {
  typedef hollow_iterator_base<
      container_type, std::vector<bool>, typename container_type::iterator> parent_class;
  hollow_iterator() : parent_class(NULL, NULL, typename container_type::iterator())  { }
  hollow_iterator(typename parent_class::container* c,
                  typename parent_class::presence* p,
                  typename parent_class::iterator it)
     : parent_class(c, p, it) { }
};

template <typename container_type>
struct hollow_const_iterator : public hollow_iterator_base<
    const container_type, const std::vector<bool>, typename container_type::const_iterator> {
  typedef hollow_iterator_base<
      const container_type, const std::vector<bool>, typename container_type::const_iterator> parent_class;
  typedef hollow_const_iterator<container_type> self_type;
  typedef hollow_iterator<container_type> non_const_type;
  hollow_const_iterator(non_const_type rhs) : parent_class(rhs.c_, rhs.p_, typename container_type::const_iterator(rhs.it_)) { }
  hollow_const_iterator() : parent_class(NULL, NULL, typename container_type::iterator())  { }
  hollow_const_iterator(const typename parent_class::container* c,
                        const typename parent_class::presence* p,
                        typename parent_class::iterator it)
     : parent_class(c, p, it) { }
};

}  // namespace cxxmph

#endif  // __CXXMPH_HOLLOW_ITERATOR_H__
