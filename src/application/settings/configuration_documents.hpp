#pragma once
#include "application/json.hpp"
#include <string>

namespace mousebox::application
{
// Persisted documents keep global hardware values separate from game schemas.
Json splitApplicationDocument(const Json &normalized);
Json splitGameDocument(const Json &normalized, const std::string &gameId, const Json &binding = Json::object());
void validateGameDocument(const Json &document);
void validateApplicationDocument(const Json &document);
} // namespace mousebox::application
