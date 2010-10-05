// Minimal perfect hash abstraction implementing the BDZ algorithm

#include <vector>

#include "trigraph.h"

template <class Key, class NewRandomlySeededHashFcn = __gnu_cxx::hash<Key> >
class MPHTable {
 public:
  typedef Key key_type;
  typedef NewRandomlySeededHashFcn hasher;
  MPHTable();
  ~MPHTable();

  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end);
  cmph_uint32 index(const key_type& x) const;

 private:
  typedef std::vector<cmph_uint32> Queue;
  template<class ForwardIterator>
  struct TableBuilderState {
    ForwardIterator begin;
    ForwardIterator end;
    Queue edges_queue;
    TriGraph graph_builder;
    double c;
    cmph_uint32 m;
    cmph_uint32 n;
    cmph_uint32 k;
    cmph_uint32 ranktablesize;
  };
  int GenerateQueue(
     cmph_uint32 nedges, cmph_uint32 nvertices,
     TriGraph* graph, Queue* queue);
   void Assigning(TriGraph* graph, Queue* queue);
   void Ranking(TriGraph* graph, Queue* queue);
   cmph_uint32 Search(const StringPiece& key);
   cmph_uint32 Rank(const StringPiece& key);

  std::vector<ConnectedEdge> graph_;
};


