
#pragma once

#include <tuple>
#include <memory>
#include <fstream>
#include <string>

namespace swegl
{

inline bool ends_with(const std::string & a, const std::string & b)
{
	if (b.size() > a.size())
		return false;
	return a.substr(a.size()-b.size()) == b;
}

inline std::string to_lower(const std::string & s)
{
	std::string s_lower = s;
	std::transform(s_lower.begin(), s_lower.end(), s_lower.begin(), [](unsigned char c){ return std::tolower(c); });
	return s_lower;
}

inline std::tuple<std::unique_ptr<char[]>,size_t> read_file(std::string filename)
{
	std::ifstream t(filename);
	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	std::unique_ptr<char[]> result = std::make_unique<char[]>(size);
	t.seekg(0);
	t.read(result.get(), size);
	return {std::move(result), size};
}

} // namespace
