// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

#ifndef MCP_BASE_FIBONACCI
#define MCP_BASE_FIBONACCI

namespace base {
static int Fibonacci(int n) {
  if (n <= 0) return 0;
  if (n == 1) return 1;
  return Fibonacci(n-2) + Fibonacci(n-1);
}

}

#endif // MCP_BASE_FIBONACCI