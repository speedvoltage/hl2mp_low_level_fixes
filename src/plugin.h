#pragma once

#include <ISmmPlugin.h>

#include <cstddef>
#include <cstdint>

struct edict_t;
class IVEngineServer;
class CGlobalVars;
typedef struct funchook funchook_t;

class HL2MPLowLevelFixes final : public ISmmPlugin
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;
	bool QueryRunning(char *error, size_t maxlen) override;
	bool Pause(char *error, size_t maxlen) override;
	bool Unpause(char *error, size_t maxlen) override;
	void AllPluginsLoaded() override;

	const char *GetAuthor() override;
	const char *GetName() override;
	const char *GetDescription() override;
	const char *GetURL() override;
	const char *GetLicense() override;
	const char *GetVersion() override;
	const char *GetDate() override;
	const char *GetLogTag() override;

	void OnDissolveThink(void *instance);

private:
	bool InstallDissolveThinkFix(char *error, size_t maxlen);
	bool RemoveDissolveThinkFix(char *error, size_t maxlen);
	bool InstallEHandleResolver(char *error, size_t maxlen);
	bool IsWeaponHandleLive(uint32_t handle) const;
	int SanitizeWeaponVector(void *instance);
	void MarkFailed(const char *reason);

	IVEngineServer *engine_ = nullptr;
	CGlobalVars *globals_ = nullptr;
	funchook_t *hook_ = nullptr;
	void *dissolve_think_ = nullptr;
	void *add_weapon_ = nullptr;
	void *lookup_entity_fn_ = nullptr;
	uint32_t entity_list_ptr_addr_ = 0;
	uint32_t weapon_vector_offset_ = 0;
	uint64_t sanitized_handles_ = 0;
	bool installed_ = false;
	bool healthy_ = true;
	char failure_[256] = {};
};

extern HL2MPLowLevelFixes g_HL2MPLowLevelFixes;

PLUGIN_GLOBALVARS();
