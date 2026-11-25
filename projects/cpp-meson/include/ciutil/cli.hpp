#ifndef CIUTIL_CLI_HPP
#define CIUTIL_CLI_HPP

#include <iosfwd>
#include <string>
#include <vector>

namespace ciutil {

// args excludes the program name. returns the exit code: 0 success, 2 runtime error, 64 usage error
int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err);

}  // namespace ciutil

#endif
