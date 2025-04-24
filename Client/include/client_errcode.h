#ifndef CHRONOLOG_CLIENT_ERRCODE_H
#define CHRONOLOG_CLIENT_ERRCODE_H

namespace chronolog {

// A “scoped” enum for client-only errors:
enum class ClientErrorCode : int {
    Success        = 0,   // Success
    Unknown        = -1,  // Generic error
    InvalidArg     = -2,  // Invalid input
    NotExist       = -3,  // Missing Chronicle, Story, or property
    Acquired       = -4,  // Already acquired, cannot destroy
    NotAcquired    = -5,  // Not acquired, cannot release
    ChronicleExists= -6,  // Chronicle already exists
    NoKeepers      = -7,  // No ChronoKeepers available
    NoConnection   = -8,  // No connection to ChronoVisor
    NotAuthorized  = -9,  // Unauthorized operation
    NoPlayers      = -10  // No ChronoPlayers available
};

// Convert enum to its integer value
constexpr int to_int(ClientErrorCode e) noexcept {
    return static_cast<int>(e);
}

// Convert enum value to its name (for logging)
inline const char* to_string(ClientErrorCode e) {
    switch (e) {
    case ClientErrorCode::Success:         return "Success";
    case ClientErrorCode::Unknown:         return "Unknown";
    case ClientErrorCode::InvalidArg:      return "InvalidArg";
    case ClientErrorCode::NotExist:        return "NotExist";
    case ClientErrorCode::Acquired:        return "Acquired";
    case ClientErrorCode::NotAcquired:     return "NotAcquired";
    case ClientErrorCode::ChronicleExists: return "ChronicleExists";
    case ClientErrorCode::NoKeepers:       return "NoKeepers";
    case ClientErrorCode::NoConnection:    return "NoConnection";
    case ClientErrorCode::NotAuthorized:   return "NotAuthorized";
    case ClientErrorCode::NoPlayers:       return "NoPlayers";
    default:                               return "UnknownClientErrorCode";
    }
}

} // namespace chronolog

#endif // CHRONOLOG_CLIENT_ERRCODE_H