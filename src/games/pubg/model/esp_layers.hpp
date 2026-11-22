#pragma once
#include "control_frame.hpp"
#include <memory>
#include <set>
#include <algorithm>

namespace mousebox::control
{
struct EspTeamInfo
{
    std::optional<Identity> self;
    std::optional<int> team;
    std::vector<Identity> members;
    bool operator==(const EspTeamInfo &) const = default;
};
// Updated at Reader arrival, before UI event coalescing. Independent category
// sessions may differ; Reader instance and global epoch own the cross-layer fence.
class EspLayers
{
    std::string reader_;
    std::uint64_t epoch_ = 0;
    std::set<std::string> retired_;
    std::shared_ptr<const Frame> living_, view_;
    std::shared_ptr<const EspTeamInfo> team_;
    std::uint64_t teamWorld_ = 0, teamGameState_ = 0;
    std::string teamMap_;
    void clearTeam()
    {
        team_.reset();
        teamWorld_ = teamGameState_ = 0;
        teamMap_.clear();
    }
    void updateTeam(const Frame &frame)
    {
        const auto world = frame.presentationWorld.value_or(frame.header.world.value_or(0)),
                   game = frame.presentationGameState.value_or(0);
        if ((world && teamWorld_ && world != teamWorld_) || (game && teamGameState_ && game != teamGameState_) ||
            (!frame.verifiedMapName.empty() && !teamMap_.empty() && frame.verifiedMapName != teamMap_) ||
            frame.verifiedMapName == "TslLobby_Persistent_Main")
            clearTeam();
        // Remember identity only in a verified match scope. Coordinates, bot
        // flags and geometry always belong to the latest current category.
        if (!frame.header.current || !world || frame.verifiedMapName.empty() ||
            frame.verifiedMapName == "TslLobby_Persistent_Main")
            return;
        auto info = team_ ? *team_ : EspTeamInfo{};
        if (frame.self && frame.selfTeam && *frame.selfTeam >= 0)
        {
            if (info.team && info.team != frame.selfTeam)
                info.members.clear();
            info.self = frame.self;
            info.team = frame.selfTeam;
        }
        if (!info.team)
            return;
        for (const auto &c : frame.characters)
            if (c.team && *c.team >= 0)
            {
                const auto found = std::ranges::find(info.members, c.identity);
                if (c.team == info.team)
                {
                    if (found == info.members.end())
                        info.members.push_back(c.identity);
                }
                else if (found != info.members.end())
                    info.members.erase(found);
            }
        teamWorld_ = world;
        if (game)
            teamGameState_ = game;
        teamMap_ = frame.verifiedMapName;
        if (!team_ || *team_ != info)
            team_ = std::make_shared<const EspTeamInfo>(std::move(info));
    }

  public:
    bool accept(std::shared_ptr<const Frame> frame)
    {
        if (!frame)
            return false;
        const auto &h = frame->header;
        if (retired_.contains(h.reader))
            return false;
        if (reader_ != h.reader)
        {
            if (!reader_.empty())
                retired_.insert(reader_);
            reader_ = h.reader;
            epoch_ = h.epoch;
            living_.reset();
            view_.reset();
            clearTeam();
        }
        else if (h.epoch < epoch_)
            return false;
        else if (h.epoch > epoch_)
        {
            epoch_ = h.epoch;
            living_.reset();
            view_.reset();
            clearTeam();
        }
        if (h.category == Category::Living)
            updateTeam(*frame);
        (h.category == Category::Living ? living_ : view_) = std::move(frame);
        return true;
    }
    void invalidate(Category category)
    {
        (category == Category::Living ? living_ : view_).reset();
    }
    void clear()
    {
        living_.reset();
        view_.reset();
        reader_.clear();
        epoch_ = 0;
        retired_.clear();
        clearTeam();
    }
    const auto &living() const
    {
        return living_;
    }
    const auto &view() const
    {
        return view_;
    }
    const auto &team() const
    {
        return team_;
    }
};
} // namespace mousebox::control
