#pragma once

namespace memory::internal {
	FORCEINLINE std::string get_roblox_path() {
		return (std::filesystem::current_path() / "RobloxPlayerBeta.exe").string();
	}
}
