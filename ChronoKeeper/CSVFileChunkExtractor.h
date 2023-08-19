#ifndef CSV_FILE_CHUNK_EXTRACTOR_H
#define CSV_FILE_CHUNK_EXTRACTOR_H

#include "chrono_common/chronolog_types.h"
#include "chrono_common/KeeperIdCard.h"
#include "StoryChunkExtractor.h"



namespace chronolog
{

class CSVFileStoryChunkExtractor : public StoryChunkExtractorBase
{


public:
    CSVFileStoryChunkExtractor( KeeperIdCard const& keeper_id_card, std::string const& csv_files_root_dir);
    
    ~CSVFileStoryChunkExtractor();

    virtual void processStoryChunk( StoryChunk *);

private:
    KeeperIdCard keeperIdCard;
    std::string rootDirectory;
    

};



}
#endif
