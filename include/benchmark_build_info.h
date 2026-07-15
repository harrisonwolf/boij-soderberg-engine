#ifndef BENCHMARK_BUILD_INFO_H
#define BENCHMARK_BUILD_INFO_H

#include <string>

#ifndef BOIJ_BUILD_COMMIT
#define BOIJ_BUILD_COMMIT "unknown"
#endif
#ifndef BOIJ_BUILD_BRANCH
#define BOIJ_BUILD_BRANCH "unknown"
#endif
#ifndef BOIJ_BUILD_PROFILE
#define BOIJ_BUILD_PROFILE "unknown"
#endif
#ifndef BOIJ_BUILD_DIRTY
#define BOIJ_BUILD_DIRTY 0
#endif
#ifndef BOIJ_BUILD_COMPILER_COMMAND
#define BOIJ_BUILD_COMPILER_COMMAND "unknown"
#endif
#ifndef BOIJ_BUILD_COMPILER_VERSION
#define BOIJ_BUILD_COMPILER_VERSION "unknown"
#endif
#ifndef BOIJ_BUILD_COMPILER_FLAGS
#define BOIJ_BUILD_COMPILER_FLAGS "unknown"
#endif

namespace benchmark_build_info {

inline std::string commit() {
	return BOIJ_BUILD_COMMIT;
}

inline std::string branch() {
	return BOIJ_BUILD_BRANCH;
}

inline std::string profile() {
	return BOIJ_BUILD_PROFILE;
}

inline bool dirty() {
	return BOIJ_BUILD_DIRTY != 0;
}

inline std::string compiler_command() {
	return BOIJ_BUILD_COMPILER_COMMAND;
}

inline std::string compiler_version() {
	return BOIJ_BUILD_COMPILER_VERSION;
}

inline std::string compiler_flags() {
	return BOIJ_BUILD_COMPILER_FLAGS;
}

} // namespace benchmark_build_info

#endif
