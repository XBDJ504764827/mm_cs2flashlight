#include "module_scanner.h"

#include <cstdlib>
#include <sstream>

#if defined(_WIN32)
#include <Windows.h>
#include <Psapi.h>
#else
#include <dlfcn.h>
#include <link.h>
#endif

namespace
{
	bool ParsePattern(const std::string &signature, std::vector<int> &pattern)
	{
		std::istringstream stream(signature);
		std::string token;
		while (stream >> token)
		{
			if (token == "?" || token == "??")
			{
				pattern.push_back(-1);
				continue;
			}

			char *end = nullptr;
			const long value = std::strtol(token.c_str(), &end, 16);
			if (end == token.c_str() || *end != '\0' || value < 0 || value > 0xff)
			{
				return false;
			}
			pattern.push_back(static_cast<int>(value));
		}

		return !pattern.empty();
	}

#if !defined(_WIN32)
	struct ModuleSearchContext
	{
		uintptr_t base;
		std::vector<ModuleScanner::Region> *regions;
	};

	int FindExecutableRegions(dl_phdr_info *info, size_t, void *data)
	{
		auto *context = static_cast<ModuleSearchContext *>(data);
		if (static_cast<uintptr_t>(info->dlpi_addr) != context->base)
		{
			return 0;
		}

		for (ElfW(Half) index = 0; index < info->dlpi_phnum; ++index)
		{
			const ElfW(Phdr) &header = info->dlpi_phdr[index];
			if (header.p_type != PT_LOAD || (header.p_flags & PF_X) == 0)
			{
				continue;
			}

			context->regions->push_back({
				reinterpret_cast<const uint8_t *>(info->dlpi_addr + header.p_vaddr),
				static_cast<size_t>(header.p_memsz),
			});
		}

		return 1;
	}
#endif
}

bool ModuleScanner::Initialize(void *addressInModule)
{
	m_regions.clear();

#if defined(_WIN32)
	HMODULE module = nullptr;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(addressInModule), &module))
	{
		return false;
	}

	MODULEINFO information;
	if (!GetModuleInformation(GetCurrentProcess(), module, &information, sizeof(information)))
	{
		return false;
	}

	m_regions.push_back({static_cast<const uint8_t *>(information.lpBaseOfDll),
		static_cast<size_t>(information.SizeOfImage)});
#else
	Dl_info information;
	if (dladdr(addressInModule, &information) == 0 || information.dli_fbase == nullptr)
	{
		return false;
	}

	ModuleSearchContext context = {
		reinterpret_cast<uintptr_t>(information.dli_fbase),
		&m_regions,
	};
	dl_iterate_phdr(FindExecutableRegions, &context);
#endif

	return !m_regions.empty();
}

void *ModuleScanner::FindPattern(const std::string &signature) const
{
	std::vector<int> pattern;
	if (!ParsePattern(signature, pattern))
	{
		return nullptr;
	}

	void *match = nullptr;
	for (const Region &region : m_regions)
	{
		if (region.size < pattern.size())
		{
			continue;
		}

		for (size_t offset = 0; offset <= region.size - pattern.size(); ++offset)
		{
			bool matches = true;
			for (size_t index = 0; index < pattern.size(); ++index)
			{
				if (pattern[index] >= 0 && region.address[offset + index] != pattern[index])
				{
					matches = false;
					break;
				}
			}

			if (!matches)
			{
				continue;
			}

			if (match != nullptr)
			{
				return nullptr;
			}
			match = const_cast<uint8_t *>(region.address + offset);
		}
	}

	return match;
}
