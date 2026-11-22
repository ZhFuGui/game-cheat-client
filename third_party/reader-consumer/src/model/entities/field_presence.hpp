#pragma once
#include "model/primitives.hpp"

namespace monitor::fields {
namespace identity {
inline constexpr u64 Instance = 1, Kind = 2, Source = 4;
}
namespace spatial {
inline constexpr u64 Position = 1, Origin = 2, Root = 4, RawRoot = 8;
}
namespace character_state {
inline constexpr u64 Team = 1, Bot = 2, BotFlags = 4, PlayerStateVtable = 8, TypeRaw = 16, Type = 32;
}
namespace character_name {
inline constexpr u64 DisplayName = 1;
}
namespace character_movement {
inline constexpr u64 Velocity = 1, Component = 2, RawComponent = 4, World = 8;
}
namespace character_skeleton {
inline constexpr u64 Source = 1, Bones = 2;
}

namespace vehicle_state {
inline constexpr u64 Health = 1, MaxHealth = 2, Fuel = 4, MaxFuel = 8, Dying = 16, Broken = 32, Common = 64,
                     RawCommon = 128;
}
namespace seats {
inline constexpr u64 Slots = 1, DeclaredSlots = 2, OccupiedSlots = 4;
}
namespace name {
inline constexpr u64 RawIndex = 1, RawNumber = 2, Index = 4, Number = 8, Text = 16;
}
namespace vehicle_model {
inline constexpr u64 Class = 1, RawClass = 2, ClassName = 4, ModelKey = 8;
}
namespace pickup_owner {
inline constexpr u64 SourceObject = 1, SlotIndex = 2;
}
namespace stack {
inline constexpr u64 Quantity = 1, Durability = 2, MaxDurability = 4;
}
namespace definition {
inline constexpr u64 ItemId = 1, Identifier = 2, DisplayName = 4, DefinitionAddress = 8;
}
namespace vehicle_classification {
inline constexpr u64 Kind = 1, NativeVtable = 2, RuleId = 4;
}
namespace pickup_classification {
inline constexpr u64 NativeVtable = 1, RuleId = 2, InstanceNameRaw = 4;
}
namespace creature_classification {
inline constexpr u64 TypeRaw = 1, Species = 2, PlayerState = 4, PlayerStateVtable = 8, NativeVtable = 16,
                     InterpretationRevision = 32, InstanceNameRaw = 64;
}
namespace item_classification {
inline constexpr u64 Category = 1, Status = 2, RuleId = 4, RulesRevision = 8;
}
namespace container_classification {
inline constexpr u64 Kind = 1, NativeVtable = 2, RuleId = 4, InstanceNameRaw = 8;
}
namespace container_contents {
inline constexpr u64 Members = 1, DeclaredSlots = 2, OccupiedSlots = 4, AdmittedMembers = 8, OpeningSource = 16,
                     ClosingSource = 32;
}
namespace context_world {
inline constexpr u64 WorldState = 1, World = 2, GameStateRaw = 4, GameState = 8, LevelRaw = 16, Level = 32,
                     TimeSeconds = 64, AliveTeams = 128, WorldRootUnverified = 256, WorldCacheUsed = 512;
}
namespace context_local {
inline constexpr u64 GameInstanceRaw = 1, GameInstance = 2, LocalPlayer = 4, Controller = 8, ControlledPawn = 16,
                     LocalPlayerState = 32, LocalCharacter = 64, ReferencePosition = 128;
}
namespace context_origin {
inline constexpr u64 Value = 1;
}
} // namespace monitor::fields
