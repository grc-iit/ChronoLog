#ifndef KEEPER_STATS_MSG_H
#define KEEPER_STATS_MSG_H

#include<ostream>
#include "KeeperIdCard.h"


namespace chronolog
{

class KeeperStatsMsg
{

   KeeperIdCard	keeperIdCard; 
   uint32_t active_story_count;

public:


  KeeperStatsMsg ( KeeperIdCard const & keeper_card = KeeperIdCard{0,0,0}
		    ,	uint32_t count=0)
	: keeperIdCard(keeper_card)
        , active_story_count(count)
	{} 

  ~KeeperStatsMsg()=default;

  KeeperIdCard const& getKeeperIdCard() const 
  { return keeperIdCard; }

  uint32_t getActiveStoryCount() const 
  { return active_story_count; }

  template <typename SerArchiveT>
  void serialize( SerArchiveT& serT)
  {
      serT & keeperIdCard;
      serT & active_story_count;
  }

};

}

std::ostream & operator << (std::ostream & cout, chronolog::KeeperStatsMsg const& stats_msg)
{
  cout<<"KeeperStatsMsg{"<<stats_msg.getKeeperIdCard()<<"}";
  return cout;
}

#endif
