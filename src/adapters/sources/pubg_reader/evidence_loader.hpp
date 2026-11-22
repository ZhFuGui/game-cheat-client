#pragma once
#include "adapters/sources/pubg_reader/control_projection.hpp"
#include <filesystem>
namespace mousebox::reader
{
struct ReconstructedEvidence
{
    monitor::synchronization::codec::Binding binding;
    std::shared_ptr<const monitor::synchronization::PublicationState> state;
    control::Frame frame;
};
ReconstructedEvidence loadEvidence(const std::filesystem::path &connectionDirectory);
} // namespace mousebox::reader
