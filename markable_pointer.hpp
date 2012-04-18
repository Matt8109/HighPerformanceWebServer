#ifndef MCP_LOCK_FREE_MARKABLE_POINTER_HEADER
#define MCP_LOCK_FREE_MARKABLE_POINTER_HEADER

namespace lock_free {

// A MarkablePoiner steals the least significant bit of a pointer and
// uses it as a logical deletion mark.
//
// Mark and unmark methods generate copies of the pointer with the
// mark ativated or deactivated, respectively.
//
template<typename T>
class MarkablePointer {
public:
  static T* mark(T* const p);
  static T* unmark(T* const p);
  static bool isMarked(T* const  p);
};

template<typename T>
union PointerAndMark {
  T* const p;
  bool m:1;

  PointerAndMark(T* const ptr) : p(ptr) {}
};

template<typename T>
T* MarkablePointer<T>::mark(T* const p) {
  PointerAndMark<T> pm(p);
  pm.m = true;
  return pm.p;
}

template<typename T>
T* MarkablePointer<T>::unmark(T* const p) {
  PointerAndMark<T> pm(p);
  pm.m = false;
  return pm.p;
}

template<typename T>
bool MarkablePointer<T>::isMarked(T* const p) {
  const PointerAndMark<T> pm(p);
  return pm.m;
}

} // namespace lock_free

#endif // MCP_LOCK_FREE_MARKABLE_POINTER_HEADER
