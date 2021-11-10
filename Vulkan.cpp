#include "Vulkan.h"
#include <fstream>

unsigned int memoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties, const VkMemoryPropertyFlags& memoryProperties)
{
	for (unsigned int i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties)
			return i;
	}
	assert(("[ERROR] Memory type with specified properties not found", false));
}

int find(const std::vector<char>& string, const std::vector<char>& find, const unsigned int& start)
{
	unsigned nFoundCharacters = 0;
	for (unsigned int i = start; i < string.size(); i++)
	{
		if (string[i] == find[nFoundCharacters])
		{
			nFoundCharacters++;
			if (nFoundCharacters == find.size())
				return i - nFoundCharacters + 1;
		}
		else
			nFoundCharacters = 0;
	}
	return -1;
}

std::vector<std::vector<char>> split(const std::vector<char>& string, const std::vector<char>& split)
{
	std::vector<std::vector<char>> result;

	unsigned int beginElement = 0;
	int endElement;
	while (true)
	{
		endElement = find(string, split, beginElement);
		if (endElement == -1)
		{
			std::vector<char> subStr(string.begin() + beginElement, string.end());
			result.push_back(subStr);
			return result;
		}
		std::vector<char> subStr(string.begin() + beginElement, string.begin() + endElement);
		result.push_back(subStr);
		beginElement = endElement + split.size();
	}
}

std::vector<char> readFile(const char* directory)
{
	std::ifstream file(directory, std::ios::binary | std::ios::ate);
	assert(("[ERROR] Failed to open file", file.is_open()));
	std::vector<char> content(file.tellg());
	file.seekg(0);
	file.read(content.data(), content.size());
	file.close();
	return content;
}

void writeFile(const char* directory, std::vector<char> content)
{
	std::ofstream file(directory, std::ios::binary);
	assert(("[ERROR] Failed to open file", file.is_open()));
	file.write(content.data(), content.size());
	file.close();
}

void writeConstants(std::vector<char>& string, const std::vector<ShaderConstant>& constants)
{
	for (const ShaderConstant& constant : constants)
	{
		unsigned int nameLength = strlen(constant.name);
		unsigned int valueLength = strlen(constant.value);
		unsigned int writeBegin = find(string, std::vector<char>(constant.name, constant.name + nameLength)) + nameLength + 1;
		unsigned int end = find(string, { '\n' }, writeBegin);
		string.erase(string.begin() + writeBegin, string.begin() + end - 1);
		string.insert(string.begin() + writeBegin, constant.value, constant.value + valueLength);
	}
}

template <>
std::vector<char> serialize(const std::string& string)
{
	std::vector<char> result;

	std::vector<char> vecData = serialize((unsigned int)string.size());
	result.insert(result.end(), vecData.begin(), vecData.end());

	result.insert(result.end(), string.c_str(), string.c_str() + string.size());
	return result;
}

template <>
void deserialize(const std::vector<char>& vecData, std::string& write)
{
	write = std::string(vecData.data(), vecData.data() + vecData.size());
}