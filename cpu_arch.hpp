#ifndef MCP_BASE_CPU_ARCH_HEADER
#define MCP_BASE_CPU_ARCH_HEADER

namespace base {

// Cache architectural properties that are relevant to software
// optimization.
class CacheArch {
public:
  // We may want to align data structures to cache line boundaries.
  // The line size is fixed now but it can be obtained
  // programmatically in some platforms -- not portably so.
  static const int LINE_SIZE = 64;
};

}  // namescape base
#endif // MCP_BASE_CPU_ARCH_HEADER
