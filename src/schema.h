#ifndef CS2_FLASHLIGHT_SCHEMA_H
#define CS2_FLASHLIGHT_SCHEMA_H

#include <string>
#include <unordered_map>

class ISchemaSystem;
class CSchemaSystemTypeScope;

class SchemaResolver
{
public:
	explicit SchemaResolver(ISchemaSystem *schemaSystem = nullptr);
	void SetSchemaSystem(ISchemaSystem *schemaSystem);
	int FindOffset(const char *className, const char *fieldName);

private:
	CSchemaSystemTypeScope *GetServerTypeScope() const;

	ISchemaSystem *m_schemaSystem;
	std::unordered_map<std::string, int> m_offsets;
};

#endif // CS2_FLASHLIGHT_SCHEMA_H
