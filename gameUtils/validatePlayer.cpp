#define UNICODE
#define _UNICODE

#include <windows.h>
#include <string>
#include <regex>

#include "offsets/offsets.hpp"
#include "process/utils.hpp"
#include "utils/strings.hpp"
#include "utils/getChildren.hpp"

#include "gameUtils/getWorkSpace.hpp"
#include "gameUtils/getPlayers.hpp"
#include "gameUtils/getHumanoidOfPlayer.hpp"
#include "gameUtils/getCharacterOfPlayer.hpp"

#include "memory/memory.hpp"

struct vec3 {
    float x;
    float y;
    float z;
};

bool isValidPlayer(const std::string& name, HANDLE hProcess, const std::vector<uintptr_t>& workSpaceChildren) {
    int indexPlayerChar = findFirstChildOfByName(workSpaceChildren, hProcess, name);
    if (indexPlayerChar == -1) return false;

    std::vector<uintptr_t> playerCharacterChildren = getChildren(hProcess, workSpaceChildren[indexPlayerChar]);
    if (playerCharacterChildren.empty()) return false;

    int headIndex = findFirstChildOfByName(playerCharacterChildren, hProcess, "Head");
    if (headIndex == -1) return false;

    uintptr_t headPrimitive = ReadMemory<uintptr_t>(hProcess, playerCharacterChildren[headIndex] + Offsets::BasePart::Primitive);
    if (headPrimitive == 0) return false;

    vec3 headPos = ReadMemory<vec3>(hProcess, headPrimitive + Offsets::BasePart::Position);
    if (!headPos.x || !headPos.y || !headPos.z) return false;
    if (headPos.x == 0 && headPos.y == 0 && headPos.z == 0) return false;

    std::string s = trim(name);
    if (s == "Hello" || s == "World" || s.empty()) return false;

    if (s.size() < 3 || s.size() > 20) return false;
    if (s.front() == '_' || s.back() == '_') return false;

    int underscoreCount = 0;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return false;
        if (c == '_') underscoreCount++;
    }
    if (underscoreCount > 1) return false;

    std::string trimmedName = trim(name);

    for (unsigned char c : trimmedName) {
        if (c >= 128 || !(
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_'
        )) {
            return false;
        }
    }

    return true;
}
