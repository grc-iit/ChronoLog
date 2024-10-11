#ifndef GRAPHER_STATS_MSG_H
#define GRAPHER_STATS_MSG_H

#include <iostream>
#include "GrapherIdCard.h"


namespace chronolog
{

class GrapherStatsMsg
{

    GrapherIdCard grapherIdCard;
    uint32_t active_story_count;

public:


    GrapherStatsMsg(GrapherIdCard const & grapher_card = GrapherIdCard{0, 0, 0}, uint32_t count = 0)
        : grapherIdCard(grapher_card)
        , active_story_count(count)
    {}

    ~GrapherStatsMsg() = default;

    GrapherIdCard const & getGrapherIdCard() const
    { return grapherIdCard; }

    uint32_t getActiveStoryCount() const
    { return active_story_count; }

    template <typename SerArchiveT>
    void serialize(SerArchiveT &serT)
    {
        serT & grapherIdCard;
        serT & active_story_count;
    }

};

}

inline std::ostream &operator<<(std::ostream &out, chronolog::GrapherStatsMsg const &stats_msg)
{
    out << "GrapherStatsMsg{" << stats_msg.getGrapherIdCard() << "}";
    return out;
}

#endif