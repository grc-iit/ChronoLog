#ifndef CHRONOLOG_SERVER_ERRCODE_H
#define CHRONOLOG_SERVER_ERRCODE_H

#include <string>
#include <ostream>

#include "../../../Client/include/client_errcode.h"

namespace chronolog {

// A “scoped” enum just for server‐only errors:
enum class ServerErrorCode : int {
    StoryExists               = -101,  // Specified Story exists, cannot be created/renamed to
    ArchiveExists             = -102,  // Specified Archive exists, cannot be created/renamed to
    ChroniclePropertyFull     = -103,  // Property list of Chronicle is full, cannot add new property
    StoryPropertyFull         = -104,  // Property list of Story is full, cannot add new property
    ChronicleMetadataFull     = -105,  // Metadata list of Chronicle is full, cannot add new property
    StoryMetadataFull         = -106,  // Metadata list of Story is full, cannot add new property
    InvalidConfig             = -107,  // Invalid configuration, cannot be created
    StoryChunkExists          = -108,  // Specified Story chunk exists, cannot be created
    ChronicleDirNotExist      = -109,  // Chronicle directory does not exist
    StoryFileNotExist         = -110,  // Story file does not exist
    StoryChunkDatasetNotExist = -111,  // Story chunk dataset does not exist
    StoryChunkExtractionError = -112   // Error in extracting Story chunk in ChronoKeeper
};

// Convert enum to its integer value
constexpr int to_int(ServerErrorCode e) noexcept {
return static_cast<int>(e);
}

// Convert enum value to its name (for logging)
inline const char* to_string(ServerErrorCode e) {
    switch (e) {
    case ServerErrorCode::StoryExists:               return "StoryExists";
    case ServerErrorCode::ArchiveExists:             return "ArchiveExists";
    case ServerErrorCode::ChroniclePropertyFull:     return "ChroniclePropertyFull";
    case ServerErrorCode::StoryPropertyFull:         return "StoryPropertyFull";
    case ServerErrorCode::ChronicleMetadataFull:     return "ChronicleMetadataFull";
    case ServerErrorCode::StoryMetadataFull:         return "StoryMetadataFull";
    case ServerErrorCode::InvalidConfig:             return "InvalidConfig";
    case ServerErrorCode::StoryChunkExists:          return "StoryChunkExists";
    case ServerErrorCode::ChronicleDirNotExist:      return "ChronicleDirNotExist";
    case ServerErrorCode::StoryFileNotExist:         return "StoryFileNotExist";
    case ServerErrorCode::StoryChunkDatasetNotExist: return "StoryChunkDatasetNotExist";
    case ServerErrorCode::StoryChunkExtractionError: return "StoryChunkExtractionError";
    default:                                         return "UnknownServerErrorCode";
    }
}


} // namespace chronolog

#endif // CHRONOLOG_SERVER_ERRCODE_H