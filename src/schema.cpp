#include "schema.h"

#include <schemasystem/schemasystem.h>
#include <schemasystem/schematypes.h>

SchemaResolver::SchemaResolver(ISchemaSystem *schemaSystem) :
	m_schemaSystem(schemaSystem)
{
}

void SchemaResolver::SetSchemaSystem(ISchemaSystem *schemaSystem)
{
	m_schemaSystem = schemaSystem;
	m_offsets.clear();
}

CSchemaSystemTypeScope *SchemaResolver::GetServerTypeScope() const
{
	if (m_schemaSystem == nullptr)
	{
		return nullptr;
	}

#if defined(_WIN32)
	return m_schemaSystem->FindTypeScopeForModule("server.dll");
#else
	return m_schemaSystem->FindTypeScopeForModule("libserver.so");
#endif
}

int SchemaResolver::FindOffset(const char *className, const char *fieldName)
{
	const std::string cacheKey = std::string(className) + "::" + fieldName;
	const auto cached = m_offsets.find(cacheKey);
	if (cached != m_offsets.end())
	{
		return cached->second;
	}

	CSchemaSystemTypeScope *scope = GetServerTypeScope();
	if (scope == nullptr)
	{
		return -1;
	}

	SchemaClassInfoData_t *classInfo = scope->FindDeclaredClass(className).Get();
	if (classInfo == nullptr)
	{
		return -1;
	}

	for (uint16 index = 0; index < classInfo->m_nFieldCount; ++index)
	{
		const SchemaClassFieldData_t &field = classInfo->m_pFields[index];
		if (field.m_pszName != nullptr && std::string(field.m_pszName) == fieldName)
		{
			m_offsets.emplace(cacheKey, field.m_nSingleInheritanceOffset);
			return field.m_nSingleInheritanceOffset;
		}
	}

	return -1;
}
