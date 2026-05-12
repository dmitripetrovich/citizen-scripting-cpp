#include <resource_sdk/SDK.h>
#include <memory>
#include <unordered_map>

static std::string extractId(const std::string& source)
{
    auto pos = source.find(':');
    return pos != std::string::npos ? source.substr(pos + 1) : source;
}

FXCPP_RESOURCE
{
    auto pending = std::make_shared<std::unordered_map<std::string, std::string>>();
    auto players = std::make_shared<std::unordered_map<std::string, std::string>>();

    std::string version = fx::getResourceMetadata("version");
    if (!version.empty())
        fx::trace("Resource version: %s\n", version.c_str());

    fx::trace("Resource started.\n");

    fx::onStop([players]() {
        fx::trace("Resource stopping, %zu players tracked.\n", players->size());
    });

    fx::on("playerConnecting", [pending](const std::string& source, fx::EventArgs args) {
        std::string name = args.get<std::string>(0);
        (*pending)[extractId(source)] = name;
    });

    fx::on("playerJoining", [pending, players](const std::string& source, fx::EventArgs args) {
        std::string oldId = args.get<std::string>(0);
        auto it = pending->find(oldId);
        std::string name = (it != pending->end()) ? it->second : "Unknown";
        if (it != pending->end()) pending->erase(it);
        std::string id = extractId(source);
        (*players)[id] = name;
        int serverId = std::stoi(id);
        fx::setTimeout(0, [serverId, name]() {
            fx::setPlayerState(serverId, "name", fx::json::makeString(name));
        });
        fx::trace("%s (%s) has connected. Players Online: %zu\n", name.c_str(), id.c_str(), players->size());
    });

    fx::on("playerDropped", [players](const std::string& source, fx::EventArgs args) {
        std::string reason = args.get<std::string>(0);
        std::string id = extractId(source);
        std::string name = players->count(id) ? (*players)[id] : "Unknown";
        players->erase(id);
        fx::trace("%s (%s) has disconnected (%s). Players Online: %zu\n", name.c_str(), id.c_str(), reason.c_str(), players->size());
    });

    fx::onNet("chatMessage", [players](const std::string& source, fx::EventArgs args) {
        std::string id = args.size() > 0 ? std::to_string(args.get<int>(0)) : extractId(source);
        std::string name = args.size() > 1 ? args.get<std::string>(1) : "Unknown";
        std::string message = args.size() > 2 ? args.get<std::string>(2) : "";
        fx::trace("%s (%s) has sent a chat message: %s\n", name.c_str(), id.c_str(), message.c_str());
    });

    fx::onCommand("players", [players](const std::string&, const std::vector<std::string>&) {
        fx::trace("--- Online Players (%zu) ---\n", players->size());
        for (const auto& [src, name] : *players)
            fx::trace("  [%s] %s\n", extractId(src).c_str(), name.c_str());
        fx::trace("----------------------------\n");
    });

    fx::onCommand("statebag", [](const std::string&, const std::vector<std::string>& args) {
        fx::json::Value count = fx::getGlobalState("playerCount");
        fx::trace("Global playerCount: %d\n", count.asInt());

        auto keys = fx::getStateBagKeys("global");
        fx::trace("Global state bag keys (%zu):\n", keys.size());
        for (const auto& key : keys)
            fx::trace("  - %s\n", key.c_str());

        if (args.size() > 0)
        {
            int id = std::stoi(args[0]);
            fx::json::Value name = fx::getPlayerState(id, "name");
            bool hasName = fx::stateBagHasKey("player:" + args[0], "name");
            fx::trace("Player %d: name=%s (hasKey=%s)\n", id, name.isNull() ? "(null)" : name.asStr().c_str(), hasName ? "true" : "false");
        }
    });

    fx::onCommand("kvp", [](const std::string&, const std::vector<std::string>& args) {
        if (args.size() >= 2)
        {
            fx::setKvp(args[0], args[1]);
            fx::trace("KVP set: %s = %s\n", args[0].c_str(), args[1].c_str());
        }
        else if (args.size() == 1)
        {
            std::string val = fx::getKvpString(args[0]);
            fx::trace("KVP get: %s = %s\n", args[0].c_str(), val.empty() ? "(empty)" : val.c_str());
        }
        else
        {
            auto keys = fx::findKvp("");
            fx::trace("All KVP keys (%zu):\n", keys.size());
            for (const auto& k : keys)
                fx::trace("  - %s = %s\n", k.c_str(), fx::getKvpString(k).c_str());
        }
    });

    fx::createThread([players]() -> fx::ScriptTask {
        while (true)
        {
            fx::setGlobalState("playerCount", fx::json::makeInt(static_cast<int>(players->size())));
            co_await fx::Wait(30000);
        }
    });

    fx::addExport("getPlayerCount", [players](fx::EventArgs) -> fx::json::Value {
        return fx::json::makeInt(static_cast<int>(players->size()));
    });

    fx::addExport("getPlayerName", [players](fx::EventArgs args) -> fx::json::Value {
        std::string src = args.get<std::string>(0);
        auto it = players->find(src);
        if (it != players->end())
            return fx::json::makeString(it->second);
        return fx::json::makeNull();
    });
}
