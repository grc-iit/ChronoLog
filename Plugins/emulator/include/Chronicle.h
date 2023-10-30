#ifndef __CHRONICLE_H_
#define __CHRONICLE_H_

#include <string>
#include <cmath>
#include <vector>

class Story
{
   private :
	 std::string name;
	 int acquisition_count;
   public:
	Story()
	{
	   acquisition_count = 0;
	}
	~Story()
	{
	}	
	void increment_acquisition_count(int a)
	{
	   acquisition_count++;
	}
	void decrement_acquisition_count(int &a)
	{
	   acquisition_count--;
	}
	void setname(std::string &s)
	{
		name = s;
	}
	std::string &getname()
	{
		return name;
	}
};



class Chronicle
{
   private:
	std::string name;
	std::vector<Story> stories;
	int acquisition_count;

   public:
	Chronicle()
	{
	   acquisition_count=0;

	}
	~Chronicle()
	{

	}
	void setname(std::string &s)
	{
		name = s;
	}
	std::string &getname()
	{
		return name;
	}
	std::vector<Story> &get_stories()
	{
	   return stories;
	}
	void increment_acquisition_count(int &a)
	{
		acquisition_count++;
	}
	void decrement_acquisition_count(int &a)
	{
		acquisition_count--;
	}
	int get_acquisition_count(int &a)
	{
		return acquisition_count;
	}
	bool add_story_to_chronicle(std::string &story_name)
	{
	    bool found = false;
	    for(int i=0;i<stories.size();i++)
	    {
		    if(stories[i].getname().compare(story_name)==0)
		    {
			    found = true; break;
		    }
	    }

	    bool ret = false;
	    if(!found)
	    {
		    Story s;
		    s.setname(story_name);
		    stories.push_back(s);
		    ret = true;
	    }
	    return ret;
	}
	bool acquire_story(std::string &story_name)
	{
		bool found = false;
		for(int i=0;i<stories.size();i++)
		{
			if(stories[i].getname().compare(story_name)==0)
			{
			    found = true;
			    int a = 1;
			    stories[i].increment_acquisition_count(a);
			    break;
			}
		}
		return found;
	}
	bool release_story(std::string &story_name)
	{
		bool found = false;
		for(int i=0;i<stories.size();i++)
		{
			if(stories[i].getname().compare(story_name)==0)
			{
			   found = true;
			   int a = 1;
			   stories[i].decrement_acquisition_count(a);
			   break;
			}
		}
		return found;
	}

};

#endif
