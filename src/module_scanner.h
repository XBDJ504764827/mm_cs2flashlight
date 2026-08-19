#ifndef CS2_FLASHLIGHT_MODULE_SCANNER_H
#define CS2_FLASHLIGHT_MODULE_SCANNER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class ModuleScanner
{
public:
	struct Region
	{
		const uint8_t *address;
		size_t size;
	};

	bool Initialize(void *addressInModule);
	void *FindPattern(const std::string &signature) const;

private:
	std::vector<Region> m_regions;
};

#endif // CS2_FLASHLIGHT_MODULE_SCANNER_H
