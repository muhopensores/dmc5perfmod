#include "hacklib/Main.h"
#include "hacklib/ConsoleEx.h"
#include "hacklib/D3DDeviceFetcher.h"
#include "hacklib/Hooker.h"
#include "hacklib/PatternScanner.h"
#include "hacklib/Patch.h"
#include "hacklib/Input.h"
#include "util/ini.h"
#include <d3d11.h>
#include <cstdio>

#define naked __declspec( naked )
/*#ifdef __INTEL_COMPILER
#define naked __declspec( naked )
#endif*/
#ifdef __clang__
#define naked __attribute__((naked))
#endif
//#define naked __declspec( naked )
hl::StaticInit<class MyMain> g_main;

extern "C" {

	HMODULE g_d3d11 = NULL;

	uintptr_t probe_jmpback   = NULL;
	uintptr_t lod_jmpback     = NULL;
	uintptr_t shadow_jmpback  = NULL;
	uintptr_t ao_jmpback      = NULL;
	uintptr_t cubemap_jmpback = NULL;
	uintptr_t fog_jmpback     = NULL;
	uintptr_t ibl_jmpback     = NULL;

	bool g_enableProbes   = false;
	bool g_enableLods     = false;
	bool g_enableAo       = false;
	bool g_enableCubemaps = false;
	bool g_enableFog      = false;
	bool g_enableIbl      = false;
}

naked void probe_detour() {
	__asm {
		//pushfq
		cmp BYTE PTR [g_enableProbes], 0
		je skipWrite
		mov BYTE PTR [rbx+150h], 0
	skipWrite:
		//popfq
		cmp [rbx+150h], r13b
		jmp QWORD PTR [probe_jmpback]
	}
}

naked void lod_detour() {
	__asm {
		pushfq
		cmp BYTE PTR [g_enableLods], 0
		je skipWrite
		mov BYTE PTR [rcx+250h], 4
		mov BYTE PTR [rcx+24Ch], 1
	skipWrite:
		popfq
		movzx eax,byte ptr [rcx+250h]
		jmp QWORD PTR [lod_jmpback]
	}
}

naked void shadow_detour() {
	__asm {
		nop
		nop
		nop
		nop
		mov BYTE PTR [rbx+179h], 00
	}
}

naked void shadow_detour2() {
	__asm {
		mov BYTE PTR [r14+179h], 00
	}
}

naked void ao_detour() {
	__asm {
		pushfq
		cmp BYTE PTR[g_enableAo], 0
		je skipWrite
		mov BYTE PTR[rdi + 34h], 0 ;//via.render.AmbientOcclusionGeometry->Enable
	skipWrite:
		popfq
		movss xmm8,[rdi+34h]
		jmp QWORD PTR [ao_jmpback]
	}
}

naked void cubemap_detour() {
	__asm {
		//pushfq
		cmp BYTE PTR [g_enableCubemaps], 0
		je skipWrite
		mov BYTE PTR [rbx+120h],00 //via.render.LocalCubemap->Enabled
	skipWrite:
		//popfq
		cmp BYTE PTR [rbx+120h],00
		jmp QWORD PTR [cubemap_jmpback]
	}
}
naked void fog_detour() {
	__asm {
		//pushfq
		cmp BYTE PTR [g_enableFog], 0
		je skipWrite
		mov BYTE PTR [rbx+81h], 0 //via.render.Fog
	skipWrite:
		//popfq
		cmp [rbx+81h], r12b
		jmp QWORD PTR [fog_jmpback]
	}
}

naked void ibl_detour() {
	__asm {
		mov BYTE PTR [rbx+31h], 00
	}
}

#pragma region Proxy
struct mfreadwrite_dll {
	HMODULE dll;
	FARPROC oDllCanUnloadNow;
	FARPROC oDllGetClassObject;
	FARPROC oMFCreateSinkWriterFromMediaSink;
	FARPROC oMFCreateSinkWriterFromURL;
	FARPROC oMFCreateSourceReaderFromByteStream;
	FARPROC oMFCreateSourceReaderFromMediaSource;
	FARPROC oMFCreateSourceReaderFromURL;
} mfreadwrite;

extern "C" {
	void fDllCanUnloadNow() { __asm jmp[mfreadwrite.oDllCanUnloadNow] }
	void fDllGetClassObject() { __asm jmp[mfreadwrite.oDllGetClassObject] }
	void fMFCreateSinkWriterFromMediaSink() { __asm jmp[mfreadwrite.oMFCreateSinkWriterFromMediaSink] }
	void fMFCreateSinkWriterFromURL() { __asm jmp[mfreadwrite.oMFCreateSinkWriterFromURL] }
	void fMFCreateSourceReaderFromByteStream() { __asm jmp[mfreadwrite.oMFCreateSourceReaderFromByteStream] }
	void fMFCreateSourceReaderFromMediaSource() { __asm jmp[mfreadwrite.oMFCreateSourceReaderFromMediaSource] }
	void fMFCreateSourceReaderFromURL() { __asm jmp[mfreadwrite.oMFCreateSourceReaderFromURL] }
}

void setupFunctions() {
	mfreadwrite.oDllCanUnloadNow = GetProcAddress(mfreadwrite.dll, "DllCanUnloadNow");
	mfreadwrite.oDllGetClassObject = GetProcAddress(mfreadwrite.dll, "DllGetClassObject");
	mfreadwrite.oMFCreateSinkWriterFromMediaSink = GetProcAddress(mfreadwrite.dll, "MFCreateSinkWriterFromMediaSink");
	mfreadwrite.oMFCreateSinkWriterFromURL = GetProcAddress(mfreadwrite.dll, "MFCreateSinkWriterFromURL");
	mfreadwrite.oMFCreateSourceReaderFromByteStream = GetProcAddress(mfreadwrite.dll, "MFCreateSourceReaderFromByteStream");
	mfreadwrite.oMFCreateSourceReaderFromMediaSource = GetProcAddress(mfreadwrite.dll, "MFCreateSourceReaderFromMediaSource");
	mfreadwrite.oMFCreateSourceReaderFromURL = GetProcAddress(mfreadwrite.dll, "MFCreateSourceReaderFromURL");
}

#pragma comment(linker, "/EXPORT:DllCanUnloadNow=fDllCanUnloadNow")
#pragma comment(linker, "/EXPORT:DllGetClassObject=fDllGetClassObject")
#pragma comment(linker, "/EXPORT:MFCreateSinkWriterFromMediaSink=fMFCreateSinkWriterFromMediaSink")
#pragma comment(linker, "/EXPORT:MFCreateSinkWriterFromURL=fMFCreateSinkWriterFromURL")
#pragma comment(linker, "/EXPORT:MFCreateSourceReaderFromByteStream=fMFCreateSourceReaderFromByteStream")
#pragma comment(linker, "/EXPORT:MFCreateSourceReaderFromMediaSource=fMFCreateSourceReaderFromMediaSource")
#pragma comment(linker, "/EXPORT:MFCreateSourceReaderFromURL=fMFCreateSourceReaderFromURL")

void failed() {
	MessageBox(0, "Unable to load the original mfreadwrite.dll. Please report this to the developer.", "dmc5perfmod", 0);
	ExitProcess(0);
}

#pragma endregion

class MyMain : public hl::Main
{
public:
    bool init() override
    {
		wchar_t buffer[MAX_PATH]{ 0 };
		if (GetSystemDirectoryW(buffer, MAX_PATH) != 0) {
			// Load the original dinput8.dll
			if ((mfreadwrite.dll = LoadLibraryW((std::wstring{ buffer } + L"\\mfreadwrite.dll").c_str())) == NULL) {
				failed();
			}
			setupFunctions();
		}
		else {
			failed();
		}

		mINI::INIFile file("performance.ini");
		mINI::INIStructure ini;
		file.read(ini);
		
		// g_enableSomething is for a specific patches, but make it more clear
		// for end users in the ini file.
		g_enableProbes   = ini["options"]["disable_probes"]   != "0";
		g_enableLods     = ini["options"]["disable_lods"]     != "0";
		m_enableShadows  = ini["options"]["disable_shadows"]  != "0";
		g_enableAo       = ini["options"]["disable_ao"]       != "0";
		g_enableCubemaps = ini["options"]["disable_cubemaps"] != "0";
		g_enableFog      = ini["options"]["disable_fog"]      != "0";
		m_enableIbl      = ini["options"]["disable_ibl"]      != "0";

		uintptr_t probesLoc = hl::FindPattern("44 38 AB 50 01 00 00");
		auto m_probeHook = m_hooker.hookJMP(probesLoc, 7, &probe_detour);
		probe_jmpback = probesLoc + 7;

		uintptr_t lodLoc = hl::FindPattern("0F B6 81 50 02 00 00");
		auto m_lodHook = m_hooker.hookJMP(lodLoc, 7, &lod_detour);
		lod_jmpback = lodLoc + 7;

		//m_shadowLoc = hl::FindPattern("84 C0 74 07 C6 83 ?? ?? ?? ?? ?? 80 BB ?? ?? ?? ?? ?? 74 1A 80 BB ?? ?? ?? ?? ??");
		m_shadowLoc = hl::FindPattern("41 80 BE 79 01 00 00 00 ?? ?? ?? ?? ?? 94 00 00 00");
		toggleShadowPatch(m_enableShadows);

		uintptr_t aoLoc = hl::FindPattern("F3 44 0F 10 47 34 44 38 6F 30");
		auto m_aoHook = m_hooker.hookJMP(aoLoc, 6, &ao_detour);
		ao_jmpback = aoLoc + 6;

		uintptr_t cubemapsLoc = hl::FindPattern("80 BB ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 48 8B 73 10 80 7E 13 00");
		auto m_cubemapHook = m_hooker.hookJMP(cubemapsLoc, 7, &cubemap_detour);
		cubemap_jmpback = cubemapsLoc + 7;

		uintptr_t fogLoc = hl::FindPattern("44 38 A3 ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 5B 20 E8 ?? ?? ?? ?? 48 85 DB 75 C6");
		auto m_fogHook = m_hooker.hookJMP(fogLoc, 7, &fog_detour);
		fog_jmpback = fogLoc + 7;

		//uintptr_t iblLoc = hl::FindPattern("48 8B 7B 10 80 7F 13 00 7D 08 48 8B CF E8 ?? ?? ?? ?? 80 7F 13 01 75 0A 80 7B 31 00");
		m_iblLoc = hl::FindPattern("0F 1F 40 00 48 8B 7B 10 80 7F 13 00 7D 08 48 8B CF E8 ?? ?? ?? ?? 80 7F 13 01 75 0A 80 7B 31 00");
		toggleIBLPatch(m_enableIbl);
		//
		//auto m_iblHook = m_hooker.hookJMP(iblLoc, 8, &ibl_detour);
#if 0
#endif
		return true;
    }

	void toggleIBLPatch(bool value) {
		if (value) {
			m_iblPatch.apply(m_iblLoc, (const char*)&ibl_detour, 4);
		}
		else {
			m_iblPatch.revert();
		}
	}

	void toggleShadowPatch(bool value) {
		if (value) {
			//m_shadowPatch.apply(m_shadowLoc, (const char*)&shadow_detour, 11);
			m_shadowPatch.apply(m_shadowLoc, (const char*)&shadow_detour2, 8);
		}
		else {
			m_shadowPatch.revert();
		}
	}

	bool step() override
	{
		m_inp.update();

		if (m_inp.isDown(VK_CONTROL))
		{
			//CTRL + F{SOMETHING} to toggle
			// hope you are enjoying my terrible formatting preferences.
			if (m_inp.wentDown(VK_F1)) { g_enableProbes   = !g_enableProbes; }
			if (m_inp.wentDown(VK_F2)) { g_enableLods     = !g_enableLods; }
			if (m_inp.wentDown(VK_F3)) { 
				m_enableShadows = !m_enableShadows;
				toggleShadowPatch(m_enableShadows);
			}
			if (m_inp.wentDown(VK_F4)) { g_enableAo       = !g_enableAo; }
			if (m_inp.wentDown(VK_F5)) { g_enableCubemaps = !g_enableCubemaps; }
			if (m_inp.wentDown(VK_F6)) { g_enableFog      = !g_enableFog; }
			if (m_inp.wentDown(VK_F7)) {
				m_enableIbl = !m_enableIbl;
				toggleIBLPatch(m_enableIbl);
			}
		}
		// terminate the mod on pressing home
		if (m_inp.wentDown(VK_HOME)) { return false; }
		
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
        return true;
    }

private:
	bool       m_enableShadows   = false;
	bool       m_enableIbl       = false;
	uintptr_t  m_iblLoc          = NULL;
	uintptr_t  m_shadowLoc       = NULL;
	hl::Patch  m_iblPatch;
	hl::Patch  m_shadowPatch;
    hl::Hooker m_hooker;
	

	/*const hl::IHook* m_probeHook;
	const hl::IHook* m_lodHook;
	const hl::IHook* m_aoHook;
	const hl::IHook* m_cubemapHook;
	const hl::IHook* m_fogHook;
	const hl::IHook* m_iblHook;*/
	
	hl::Input m_inp;
};