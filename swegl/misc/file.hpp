
#pragma once

#include <memory>
#include <fstream>
#include <string>

namespace swegl
{

std::unique_ptr<char[]> read_file(std::string filename)
{
	std::ifstream t(filename);
	t.seekg(0, std::ios::end);
	size_t size = t.tellg();
	auto result = std::make_unique<char[]>(size);
	t.seekg(0);
	t.read(result.get(), size);
	return result;
}

} // namespace
