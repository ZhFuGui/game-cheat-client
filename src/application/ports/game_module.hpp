#pragma once
#include "data_source.hpp"
#include "domain/game.hpp"
#include "presentation_frame.hpp"
#include "game_application.hpp"

namespace mousebox::application::ports
{
struct PresentationOptions
{
    render::Palette palette;
    bool english = false;
    render::EspOptions esp;
};
struct GameWindowTarget
{
    std::string executable;
    std::uint32_t process = 0;
};
class GameModule
{
  public:
    virtual ~GameModule() = default;
    virtual const games::GameDefinition &definition() const noexcept = 0;
    virtual DataSource &dataSource() noexcept = 0;
    virtual void resetWorkspace() = 0;
    virtual void setSessionEligible(bool) = 0;
    virtual void stopOutput() = 0;
    virtual void setPresentationActive(bool) {}
    virtual GameWindowTarget presentationTarget() const
    {
        return {std::string(definition().windowExecutable), 0};
    }
    virtual PresentationFrame presentation(const PresentationOptions &) = 0;
    virtual GameApplication *application() noexcept
    {
        return nullptr;
    }
};
} // namespace mousebox::application::ports
