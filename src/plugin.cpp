#include "plugin.h"

#include <eiface.h>
#include <edict.h>
#include <const.h>
#include <funchook.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#include <link.h>
#endif

namespace
{
struct RawUtlVector32
{
	uint32_t *memory;
	int allocation_count;
	int grow_size;
	int size;
	uint32_t *elements;
};

static_assert(sizeof(void *) == 4, "HL2MP - Low-Level Fixes currently targets x86 only");
static_assert(sizeof(RawUtlVector32) == 20, "Unexpected CUtlVector x86 layout");
static_assert(offsetof(RawUtlVector32, size) == 12, "Unexpected CUtlVector size offset");
static_assert(offsetof(RawUtlVector32, elements) == 16, "Unexpected CUtlVector elements offset");

struct BytePattern
{
	const uint8_t *bytes;
	const char *mask;
	size_t length;
};

#if defined(_WIN32)
const uint8_t kDissolveThinkBytes[] = {
	0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x40, 0x53, 0x56, 0x57, 0x8B, 0xF9, 0x33,
	0xF6, 0x89, 0x75, 0xFC, 0x8B, 0x97, 0, 0, 0, 0, 0x89, 0x55, 0xF8, 0x85,
	0xD2, 0x7E, 0, 0x8D, 0x49, 0x00, 0x8B, 0x87, 0, 0, 0, 0, 0x8B, 0x0C,
	0xB0, 0x85, 0xC9, 0x74, 0, 0xBA, 0xFF, 0x1F, 0x00, 0x00
};
const char kDissolveThinkMask[] = "xxxxxxxxxxxxxxxxxx????xxxxxx?xxxxx????xxxxxx?xxxxx";
#else
const uint8_t kDissolveThinkBytes[] = {
	0x55, 0x89, 0xE5, 0x57, 0x56, 0x53, 0x83, 0xEC, 0x7C, 0x8B, 0x5D, 0x08,
	0x8B, 0x83, 0, 0, 0, 0, 0x85, 0xC0, 0x89, 0x45, 0x9C, 0x0F, 0x8E, 0, 0,
	0, 0, 0xC7, 0x45, 0x90, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x7D, 0xB0, 0x83,
	0xF8, 0x01
};
const char kDissolveThinkMask[] = "xxxxxxxxxxxxxx????xxxxxxx????xxxxxxxxxxxxx";
#endif

const BytePattern kDissolveThinkPattern = {
	kDissolveThinkBytes,
	kDissolveThinkMask,
	sizeof(kDissolveThinkBytes)
};

#if !defined(_WIN32)
// Signature for CTriggerWeaponDissolve::AddWeapon(CBaseCombatWeapon*). Needed
// not to hook it, but because g_pEntityList's address is embedded directly in
// its compiled code (see InstallEHandleResolver) - that's the only reason
// this function's location matters to the fix. Verified unique.
const uint8_t kAddWeaponBytes[] = {
	0x55, 0x89, 0xE5, 0x57, 0x56, 0x53, 0x83, 0xEC, 0x2C, 0x8B, 0x4D, 0x0C,
	0x85, 0xC9, 0x0F, 0x84, 0x8C, 0x01, 0x00, 0x00, 0x8B, 0x45, 0x0C, 0x8B,
	0x00, 0x8B, 0x40, 0x0C, 0x3D, 0, 0, 0, 0, 0x0F, 0x85, 0xF9,
	0x01, 0x00, 0x00, 0x8B, 0x45, 0x0C, 0x05, 0x60, 0x03, 0x00, 0x00, 0x8B,
	0x10, 0x8B, 0x45, 0x08, 0x8B, 0x80, 0x04, 0x05, 0x00, 0x00, 0x85, 0xC0,
};
const char kAddWeaponMask[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxx";
const BytePattern kAddWeaponPattern = {
	kAddWeaponBytes,
	kAddWeaponMask,
	sizeof(kAddWeaponBytes)
};

// CBaseEntityList::LookupEntity(const CBaseHandle&) const - the game's own,
// authoritative EHANDLE resolution. m_pWeapons stores EHANDLE-packed values on
// this build (not raw CBaseCombatWeapon* as the reference source shows), so
// this is the correct way to validate a stored handle is still live: call the
// real LookupEntity rather than reimplement its lookup table by hand. Clean
// signature, no relocations in this span.
const uint8_t kLookupEntityBytes[] = {
	0x55, 0x89, 0xE5, 0x8B, 0x45, 0x0C, 0x8B, 0x10, 0x85, 0xD2, 0x74, 0x2C,
	0x83, 0xFA, 0xFF, 0x0F, 0xB7, 0xC2, 0xB9, 0xFF, 0x1F, 0x00, 0x00, 0x0F,
	0x44, 0xC1, 0xC1, 0xEA, 0x10, 0x31, 0xC9, 0xC1, 0xE0, 0x04, 0x03, 0x45,
	0x08, 0x39, 0x50, 0x08, 0x75, 0x03, 0x8B, 0x48, 0x04, 0x89, 0xC8, 0x5D,
};
const char kLookupEntityMask[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const BytePattern kLookupEntityPattern = {
	kLookupEntityBytes,
	kLookupEntityMask,
	sizeof(kLookupEntityBytes)
};
#endif

bool Matches(const uint8_t *address, const BytePattern &pattern)
{
	for (size_t i = 0; i < pattern.length; ++i)
	{
		if (pattern.mask[i] == 'x' && address[i] != pattern.bytes[i])
			return false;
	}
	return true;
}

const uint8_t *ScanRange(const uint8_t *begin, size_t length, const BytePattern &pattern, int &matches)
{
	const uint8_t *result = nullptr;
	if (length < pattern.length)
		return nullptr;

	for (size_t i = 0; i <= length - pattern.length; ++i)
	{
		if (!Matches(begin + i, pattern))
			continue;
		result = begin + i;
		++matches;
	}
	return result;
}

#if defined(_WIN32)
const uint8_t *FindPatternInServer(CreateInterfaceFn server_factory, const BytePattern &pattern, int &matches)
{
	HMODULE module = nullptr;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCSTR>(server_factory), &module))
		return nullptr;

	const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(module);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(reinterpret_cast<const uint8_t *>(module) + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
		return nullptr;

	const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nt);
	const uint8_t *result = nullptr;
	for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
	{
		if ((section[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
			continue;
		int local_matches = 0;
		const uint8_t *local = ScanRange(reinterpret_cast<const uint8_t *>(module) + section[i].VirtualAddress,
			section[i].Misc.VirtualSize, pattern, local_matches);
		if (local)
			result = local;
		matches += local_matches;
	}
	return result;
}
#else
struct LinuxScanContext
{
	uintptr_t module_base;
	const BytePattern *pattern;
	const uint8_t *result;
	int matches;
};

int ScanLinuxModule(struct dl_phdr_info *info, size_t, void *data)
{
	auto *context = static_cast<LinuxScanContext *>(data);
	if (static_cast<uintptr_t>(info->dlpi_addr) != context->module_base)
		return 0;

	for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i)
	{
		const ElfW(Phdr) &segment = info->dlpi_phdr[i];
		if (segment.p_type != PT_LOAD || (segment.p_flags & PF_X) == 0)
			continue;
		int local_matches = 0;
		const uint8_t *local = ScanRange(reinterpret_cast<const uint8_t *>(info->dlpi_addr + segment.p_vaddr),
			segment.p_memsz, *context->pattern, local_matches);
		if (local)
			context->result = local;
		context->matches += local_matches;
	}
	return 1;
}

const uint8_t *FindPatternInServer(CreateInterfaceFn server_factory, const BytePattern &pattern, int &matches)
{
	Dl_info info = {};
	if (dladdr(reinterpret_cast<void *>(server_factory), &info) == 0 || !info.dli_fbase)
		return nullptr;

	LinuxScanContext context = {
		reinterpret_cast<uintptr_t>(info.dli_fbase),
		&pattern,
		nullptr,
		0
	};
	dl_iterate_phdr(ScanLinuxModule, &context);
	matches = context.matches;
	return context.result;
}
#endif

#if defined(_WIN32)
using DissolveThinkFn = void (__thiscall *)(void *);
DissolveThinkFn g_OriginalDissolveThink = nullptr;
void __fastcall DetourDissolveThink(void *instance, void *)
{
	g_HL2MPLowLevelFixes.OnDissolveThink(instance);
}
#else
using DissolveThinkFn = void (*)(void *);
DissolveThinkFn g_OriginalDissolveThink = nullptr;
void DetourDissolveThink(void *instance)
{
	g_HL2MPLowLevelFixes.OnDissolveThink(instance);
}

using LookupEntityFn = void *(*)(void *entityList, const uint32_t *handle);
#endif
}

HL2MPLowLevelFixes g_HL2MPLowLevelFixes;
PLUGIN_EXPOSE(HL2MPLowLevelFixes, g_HL2MPLowLevelFixes);

bool HL2MPLowLevelFixes::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool)
{
	PLUGIN_SAVEVARS();

	int status = 0;
	engine_ = static_cast<IVEngineServer *>(ismm->GetEngineFactory(false)(INTERFACEVERSION_VENGINESERVER, &status));
	globals_ = ismm->GetCGlobals();
	if (!engine_ || !globals_)
	{
		std::snprintf(error, maxlen, "Could not acquire IVEngineServer or CGlobalVars");
		return false;
	}

	if (!InstallDissolveThinkFix(error, maxlen))
		return false;

#if !defined(_WIN32)
	if (!InstallEHandleResolver(error, maxlen))
		return false;
#endif

	META_LOG(g_PLAPI, "Loaded. DissolveThink %p; m_pWeapons offset 0x%X.", dissolve_think_, weapon_vector_offset_);
	return true;
}

bool HL2MPLowLevelFixes::Unload(char *error, size_t maxlen)
{
	if (!RemoveDissolveThinkFix(error, maxlen))
		return false;

	META_LOG(g_PLAPI, "Unloaded. Sanitized %llu stale trigger_weapon_dissolve handle(s).",
		static_cast<unsigned long long>(sanitized_handles_));
	return true;
}

bool HL2MPLowLevelFixes::QueryRunning(char *error, size_t maxlen)
{
	if (healthy_ && installed_)
		return true;
	std::snprintf(error, maxlen, "%s", failure_[0] ? failure_ : "Fix is not installed");
	return false;
}

bool HL2MPLowLevelFixes::Pause(char *error, size_t maxlen)
{
	std::snprintf(error, maxlen, "Pausing is unsupported while native detours are installed");
	return false;
}

bool HL2MPLowLevelFixes::Unpause(char *, size_t)
{
	return true;
}

void HL2MPLowLevelFixes::AllPluginsLoaded()
{
}

const char *HL2MPLowLevelFixes::GetAuthor()
{
	return "Peter Brev";
}

const char *HL2MPLowLevelFixes::GetName()
{
	return "HL2MP - Low-Level Fixes";
}

const char *HL2MPLowLevelFixes::GetDescription()
{
	return "Native crash and lifetime fixes for Half-Life 2: Deathmatch";
}

const char *HL2MPLowLevelFixes::GetURL()
{
	return "https://github.com/speedvoltage";
}

const char *HL2MPLowLevelFixes::GetLicense()
{
	return "GPLv2";
}

const char *HL2MPLowLevelFixes::GetVersion()
{
	return "1.0.0";
}

const char *HL2MPLowLevelFixes::GetDate()
{
	return __DATE__;
}

const char *HL2MPLowLevelFixes::GetLogTag()
{
	return "HL2MP-LLF";
}

bool HL2MPLowLevelFixes::InstallDissolveThinkFix(char *error, size_t maxlen)
{
	CreateInterfaceFn server_factory = g_SMAPI->GetServerFactory(false);
	int matches = 0;
	const uint8_t *target = FindPatternInServer(server_factory, kDissolveThinkPattern, matches);
	if (!target || matches != 1)
	{
		std::snprintf(error, maxlen, "CTriggerWeaponDissolve::DissolveThink signature resolved %d times", matches);
		return false;
	}

#if defined(_WIN32)
	const uint32_t count_offset = *reinterpret_cast<const uint32_t *>(target + 18);
	const uint8_t member_modrm = 0x87;
#else
	const uint32_t count_offset = *reinterpret_cast<const uint32_t *>(target + 14);
	const uint8_t member_modrm = 0x83;
#endif
	if (count_offset < 12 || count_offset > 0x4000 || (count_offset & 3) != 0)
	{
		std::snprintf(error, maxlen, "Invalid m_pWeapons count offset 0x%X", count_offset);
		return false;
	}

	const uint32_t data_offset = count_offset - 12;
	bool data_load_found = false;
	for (size_t i = 0; i + 6 <= 320; ++i)
	{
		if (target[i] != 0x8B || target[i + 1] != member_modrm)
			continue;
		if (*reinterpret_cast<const uint32_t *>(target + i + 2) == data_offset)
		{
			data_load_found = true;
			break;
		}
	}
	if (!data_load_found)
	{
		std::snprintf(error, maxlen, "Could not validate m_pWeapons data offset 0x%X", data_offset);
		return false;
	}

	weapon_vector_offset_ = data_offset;
	dissolve_think_ = const_cast<uint8_t *>(target);
	g_OriginalDissolveThink = reinterpret_cast<DissolveThinkFn>(dissolve_think_);

	hook_ = funchook_create();
	if (!hook_)
	{
		std::snprintf(error, maxlen, "funchook_create failed");
		return false;
	}

	void *original = reinterpret_cast<void *>(g_OriginalDissolveThink);
	int result = funchook_prepare(hook_, &original, reinterpret_cast<void *>(&DetourDissolveThink));
	if (result != FUNCHOOK_ERROR_SUCCESS)
	{
		std::snprintf(error, maxlen, "funchook_prepare failed: %s", funchook_error_message(hook_));
		funchook_destroy(hook_);
		hook_ = nullptr;
		return false;
	}
	g_OriginalDissolveThink = reinterpret_cast<DissolveThinkFn>(original);

	result = funchook_install(hook_, 0);
	if (result != FUNCHOOK_ERROR_SUCCESS)
	{
		std::snprintf(error, maxlen, "funchook_install failed: %s", funchook_error_message(hook_));
		funchook_destroy(hook_);
		hook_ = nullptr;
		return false;
	}

	installed_ = true;
	return true;
}

#if !defined(_WIN32)
bool HL2MPLowLevelFixes::InstallEHandleResolver(char *error, size_t maxlen)
{
	CreateInterfaceFn server_factory = g_SMAPI->GetServerFactory(false);
	if (!server_factory)
	{
		std::snprintf(error, maxlen, "Could not acquire server factory for EHandle resolver");
		return false;
	}

	int aw_matches = 0;
	const uint8_t *aw_target = FindPatternInServer(server_factory, kAddWeaponPattern, aw_matches);
	if (!aw_target || aw_matches != 1)
	{
		std::snprintf(error, maxlen, "CTriggerWeaponDissolve::AddWeapon signature resolved %d times", aw_matches);
		return false;
	}
	add_weapon_ = const_cast<uint8_t *>(aw_target);

	// g_pEntityList's address is embedded directly in AddWeapon's own compiled
	// code (confirmed via disassembly: `mov 0xe4d694, %eax` at offset 0x57 from
	// AddWeapon's start). Reading it from the already-relocated, loaded code
	// gives us the correct runtime address without needing a separate signature.
	const uint32_t kGEntityListOffsetInAddWeapon = 0x57;
	entity_list_ptr_addr_ = *reinterpret_cast<const uint32_t *>(
		reinterpret_cast<const uint8_t *>(add_weapon_) + kGEntityListOffsetInAddWeapon);
	if (entity_list_ptr_addr_ < 0x10000)
	{
		std::snprintf(error, maxlen, "g_pEntityList address looked implausible: 0x%X", entity_list_ptr_addr_);
		return false;
	}

	int le_matches = 0;
	const uint8_t *le_target = FindPatternInServer(server_factory, kLookupEntityPattern, le_matches);
	if (!le_target || le_matches != 1)
	{
		std::snprintf(error, maxlen, "CBaseEntityList::LookupEntity signature resolved %d times", le_matches);
		return false;
	}
	lookup_entity_fn_ = const_cast<uint8_t *>(le_target);

	return true;
}
#endif

bool HL2MPLowLevelFixes::RemoveDissolveThinkFix(char *error, size_t maxlen)
{
	if (!hook_)
		return true;

	if (installed_)
	{
		const int result = funchook_uninstall(hook_, 0);
		if (result != FUNCHOOK_ERROR_SUCCESS)
		{
			std::snprintf(error, maxlen, "funchook_uninstall failed: %s", funchook_error_message(hook_));
			return false;
		}
		installed_ = false;
	}

	const int result = funchook_destroy(hook_);
	if (result != FUNCHOOK_ERROR_SUCCESS)
	{
		std::snprintf(error, maxlen, "funchook_destroy failed: %s", funchook_error_message(hook_));
		return false;
	}
	hook_ = nullptr;

	return true;
}

bool HL2MPLowLevelFixes::IsWeaponHandleLive(uint32_t handle) const
{
	// m_pWeapons genuinely stores EHANDLE-packed values on this build - confirmed
	// directly from AddWeapon's own inlined HasWeapon() dedup check, which resolves
	// stored values via CBaseEntityList::LookupEntity(), not as raw CBaseCombatWeapon*
	// pointers the way the reference source shows. Validate the same way the game
	// itself does: call the real LookupEntity rather than reimplement its lookup
	// table by hand.
	if (handle == 0 || !lookup_entity_fn_ || entity_list_ptr_addr_ == 0)
		return false;

	void *entityList = *reinterpret_cast<void *const *>(static_cast<uintptr_t>(entity_list_ptr_addr_));
	if (!entityList)
		return false;

	auto lookupEntity = reinterpret_cast<LookupEntityFn>(lookup_entity_fn_);
	return lookupEntity(entityList, &handle) != nullptr;
}

int HL2MPLowLevelFixes::SanitizeWeaponVector(void *instance)
{
	if (!instance || weapon_vector_offset_ == 0)
		return -1;

	auto *vector = reinterpret_cast<RawUtlVector32 *>(reinterpret_cast<uint8_t *>(instance) + weapon_vector_offset_);

	if (vector->allocation_count < 0 || vector->allocation_count > 4096 || vector->size < 0 ||
		vector->size > vector->allocation_count || (vector->size > 0 && !vector->memory))
		return -1;

	if (vector->size > 0 && (reinterpret_cast<uintptr_t>(vector->memory) < 0x10000 ||
		(reinterpret_cast<uintptr_t>(vector->memory) & 3) != 0))
		return -1;

	if (vector->size > 0 && vector->elements != vector->memory)
		return -1;

	int write = 0;
	const int original_size = vector->size;
	for (int read = 0; read < original_size; ++read)
	{
		const uint32_t handle = vector->memory[read];
		if (!IsWeaponHandleLive(handle))
			continue;
		if (write != read)
			vector->memory[write] = handle;
		++write;
	}

	for (int i = write; i < original_size; ++i)
		vector->memory[i] = 0;
	vector->size = write;
	if (vector->memory)
		vector->elements = vector->memory;
	return original_size - write;
}

void HL2MPLowLevelFixes::OnDissolveThink(void *instance)
{
	const int removed = SanitizeWeaponVector(instance);
	if (removed < 0)
	{
		MarkFailed("Invalid CTriggerWeaponDissolve::m_pWeapons layout");
		return;
	}

	if (removed > 0)
	{
		sanitized_handles_ += static_cast<uint64_t>(removed);
		META_LOG(g_PLAPI, "Removed %d stale trigger_weapon_dissolve weapon handle(s).", removed);
	}

	g_OriginalDissolveThink(instance);
}

void HL2MPLowLevelFixes::MarkFailed(const char *reason)
{
	if (!healthy_)
		return;
	healthy_ = false;
	std::snprintf(failure_, sizeof(failure_), "%s", reason);
	META_LOG(g_PLAPI, "Fix disabled: %s", failure_);
}
