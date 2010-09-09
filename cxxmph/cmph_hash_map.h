#include <hash_map>
#include <vector>
#include <utility>  // for std::pair

#include <cmph.h>

// Save on repetitive typing.
#define CMPH_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc> 
#define CMPH_CLASS_SPEC cmph_hash_map<Key, Data, HashFcn, EqualKey, Alloc>
#define CMPH_METHOD_DECL(r, m) CMPH_TMPL_SPEC typename CMPH_CLASS_SPEC::r CMPH_CLASS_SPEC::m

template <class Key, class Data, class HashFcn = __gnu_cxx::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> > 
class cmph_hash_map {
 public:
  typedef Key key_type;
  typedef Data data_type;
  typedef std::pair<Key, Data> value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;

  typedef typename std::vector<value_type>::pointer pointer;
  typedef typename std::vector<value_type>::reference reference;
  typedef typename std::vector<value_type>::const_reference const_reference;
  typedef typename std::vector<value_type>::size_type size_type;
  typedef typename std::vector<value_type>::difference_type difference_type;
  typedef typename std::vector<value_type>::iterator iterator;
  typedef typename std::vector<value_type>::const_iterator const_iterator;

  // For making macros simpler.
  typedef void void_type;
  typedef bool bool_type;
  typedef std::pair<iterator, bool> insert_return_type;

  cmph_hash_map();
  ~cmph_hash_map();

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  size_type size() const;
  bool empty() const;
  void clear();
  void erase(iterator pos);
  void erase(const key_type& k);
  std::pair<iterator, bool> insert(const value_type& x);
  iterator find(const key_type& k);
  const_iterator find(const key_type& k) const;
  data_type& operator[](const key_type &k);

  void pack() { rehash(); }

 private:
  void rehash();
  std::vector<value_type> values_;
  cmph_t* cmph_;
  typedef typename __gnu_cxx::hash_map<Key, Data, HashFcn, EqualKey, Alloc> slack_type;
  slack_type slack_;
};

CMPH_TMPL_SPEC
bool operator==(const CMPH_CLASS_SPEC& lhs, const CMPH_CLASS_SPEC& rhs) {
  return lhs.values_ == rhs.values_;
}

CMPH_TMPL_SPEC CMPH_CLASS_SPEC::cmph_hash_map() : cmph_(NULL) {
  rehash();
}

CMPH_TMPL_SPEC CMPH_CLASS_SPEC::~cmph_hash_map() {
  if(cmph_) cmph_destroy(cmph_);
}

CMPH_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  iterator it = find(x.first);
  if (it != end()) return std::make_pair(it, false);
  values_.push_back(x);
  slack_.insert(std::make_pair(x.first, values_.size() - 1));
  if ((slack_.size() > 10 && !cmph_) ||
      (cmph_ && slack_.size() > cmph_size(cmph_) * 2)) rehash();
  it = find(x.first);
  // std::cerr << "inserted " << x.first.i_ << " at " << values_.begin() - it;
  return std::make_pair(it, true);
}

CMPH_METHOD_DECL(void_type, rehash)() {
  if (values_.empty()) return;
  slack_type().swap(slack_);
  cmph_io_adapter_t* source = cmph_io_struct_vector_adapter(
      &(values_[0]), sizeof(value_type), 0, sizeof(key_type), values_.size());
  cmph_config_t* cmph_config = cmph_config_new(source);
  cmph_config_set_algo(cmph_config, CMPH_CHD);
  //  cmph_config_set_verbosity(cmph_config, 1);
  if (cmph_) cmph_destroy(cmph_);
  cmph_ = cmph_new(cmph_config);
  cmph_config_destroy(cmph_config);
  cmph_io_struct_vector_adapter_destroy(source);
  std::vector<value_type> new_values(values_.size());
  for (int i = 0; i < values_.size(); ++i) {
    size_type id = cmph_search(cmph_, reinterpret_cast<const char*>(&(values_[i].first)), sizeof(key_type));
    new_values[id] = values_[i];
  }
  values_.swap(new_values);
}

CMPH_METHOD_DECL(iterator, begin)() { return values_.begin(); }
CMPH_METHOD_DECL(iterator, end)() { return values_.end(); }
CMPH_METHOD_DECL(const_iterator, begin)() const { return values_.begin(); }
CMPH_METHOD_DECL(const_iterator, end)() const { return values_.end(); }
CMPH_METHOD_DECL(bool_type, empty)() const { return values_.empty(); }

CMPH_METHOD_DECL(void_type, clear)() { 
  values_.clear();
  slack_.clear();
  cmph_destroy(cmph_);
  cmph_ = NULL;
}

CMPH_METHOD_DECL(void_type, erase)(iterator pos) {
  values_.erase(pos);
  rehash();
}
CMPH_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) return;
  erase(it);
}

CMPH_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  if (!slack_.empty()) {
    typename slack_type::const_iterator it = slack_.find(k);
    if (it != slack_.end()) return values_.begin() + it->second;
  }
  if (!cmph_) return end();
  size_type id = cmph_search(cmph_, reinterpret_cast<const char*>(&k),
                             sizeof(key_type));
  if (key_equal()(values_[id].first, k)) {
    return values_.begin() + id;
  }
  return end();
}
CMPH_METHOD_DECL(iterator, find)(const key_type& k) {
  if (!slack_.empty()) {
    typename slack_type::const_iterator it = slack_.find(k);
    if (it != slack_.end()) return values_.begin() + it->second;
  }
  if (!cmph_) return end();
  size_type id = cmph_search(cmph_, reinterpret_cast<const char*>(&k),
                             sizeof(key_type));
  if (key_equal()(values_[id].first, k)) {
    return values_.begin() + id;
  }
  return end();
}
  

CMPH_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(std::make_pair(k, data_type())).first->second;
}
