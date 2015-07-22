// ----------------------------------------------------------------------------
//
//  Copyright (C) 2015 Thomas Brand <tom@trellis.ch>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  This file is part of pled
//  https://github.com/7890/pled
//
//tb/150716+
// ----------------------------------------------------------------------------

#ifndef PLAYLIST_H_INCLUDED
#define PLAYLIST_H_INCLUDED

#include "ops.h"

//incremented at struct creation
uint64_t PlaylistItem_id_serial_counter=0;

#define MAX_PL_ITEMS 10000

//===============================================================================
//===============================================================================
struct PlaylistItem
{
	uint64_t id_serial;
	uint64_t nid; //original event belongs to this node

	Event *ev; //copied values, not referenced

	//=======================================================================
	PlaylistItem()
	{
		id_serial=PlaylistItem_id_serial_counter;
		PlaylistItem_id_serial_counter++;

		nid=0;
		ev=NULL;
	}

	//=======================================================================
	void print(uint64_t index)
	{
		if(ev->silent)
		{
			fprintf(stderr,"[%"PRId64"] %"PRId64" %d %"PRId64"\n"
				,index
				,ev->start_linear
				,ev->silent
				,ev->count
			);
		}
		else
		{
			fprintf(stderr,"[%"PRId64"] %"PRId64" %d %"PRId64" %"PRId64" %"PRId64"; %"PRId64"\n"
				,index
				,ev->start_linear
				,ev->silent
				,ev->count
				,ev->file_id
				,ev->start //orig
				,nid
			);
		}
	}
};//end struct PlaylistItem

//===============================================================================
//===============================================================================
struct Playlist
{
	uint64_t plitems_counter; //for (next) index

	struct PlaylistItem *plitems[MAX_PL_ITEMS];

	//=======================================================================
	void reset()
	{
		for(int i=0;i<MAX_PL_ITEMS;i++)
		{
			plitems[i]=NULL;
		}
		plitems_counter=0;
	}

	//=======================================================================
	Playlist()
	{
		reset();	
	}

	//=======================================================================
	//query after eval
	int is_empty()
	{
		if(plitems_counter==0)
		{
			return 1;
		}
		return 0;
	}

	//=======================================================================
	uint64_t count()
	{
		return plitems_counter;
	}

	//=======================================================================
	PlaylistItem * get_at_index(uint64_t index)
	{
		if(index>=0 && index < plitems_counter)
		{
			return plitems[index];
		}
		else
		{
			return NULL;
		}
	}

	//=======================================================================
	TreeNode * get_node_at_index(uint64_t index)
	{
		if(index>=0 && index < plitems_counter)
		{
			return find_node(plitems[index]->nid);
		}
		else
		{
			return NULL;
		}
	}

	//=======================================================================
	int add(PlaylistItem *pli)
	{
		plitems[plitems_counter]=pli;
		plitems_counter++;
	}

	//=======================================================================
	int add(uint64_t start_linear, uint64_t nid, uint64_t start, uint64_t count, int silent,uint64_t file_id)
	{
		PlaylistItem *pli = new PlaylistItem();

		pli->nid=nid;

		pli->ev=new Event();
		pli->ev->start_linear=start_linear;
		pli->ev->start=start;
		pli->ev->count=count;
		pli->ev->silent=silent;
		pli->ev->file_id=file_id;

		plitems[plitems_counter]=pli;

//		fprintf(stderr,"added to playlist, at index %"PRId64"\n",plitems_counter);
		plitems_counter++;

		return 1;
	}

	//=======================================================================
	uint64_t end()
	{
		if(is_empty())
		{
			return 0;
		}
		else
		{
			return plitems[plitems_counter-1]->ev->start_linear
				+plitems[plitems_counter-1]->ev->count;
		}
	}

	//=======================================================================
	int find_pos_index(uint64_t start_linear,int match_style)
	{
		if(plitems_counter<1)
		{
//			fprintf(stderr,"/!\\ no items in playlist\n");
			return -1;
		}

		if(start_linear<0 || start_linear > end())
		{
			fprintf(stderr,"/!\\ position outside range\n");
			return -1;
		}

		for(int i=0;i<plitems_counter;i++)
		{
			//<= can match too early (from)
			//< can match to late (to)
			if(match_style==0) //<=
			{
				if(start_linear >= plitems[i]->ev->start_linear 
					&& start_linear <= plitems[i]->ev->start_linear + plitems[i]->ev->count)
					{
						return i;
					}
				}
				else //<
				{
					if(start_linear >= plitems[i]->ev->start_linear 
						&& start_linear < plitems[i]->ev->start_linear + plitems[i]->ev->count)
					{
						return i;
					}
				}

			}//end for
		return -1;
	}//end find_pos_index()

	//=======================================================================
	virtual void print()
	{
		if(plitems_counter==0)
		{
			fprintf(stderr,"//===playlist: empty\n");
			return;
		}

		//header
		fprintf(stderr,"//===playlist: %"PRId64" items (%"PRId64"):\n//[index] pos silent count (fid start; nid)\n"
			,plitems_counter
//			,plitems[0]->ev->start_linear
			,end()
		);

		for(uint64_t i=0;i<plitems_counter;i++)
		{
			plitems[i]->print(i);
		}
		fprintf(stderr,"\n");
	}//end print()
};//end struct Playlist

//===============================================================================
//===============================================================================
struct MergedPlaylist : Playlist
{
	//=======================================================================
	void update(Playlist *pl)
	{
		reset();

		PlaylistItem *pli=NULL;
		PlaylistItem *pli_merged=NULL;

		//"copy-merge"
		uint64_t unified_silent_count=0;

		int prev_was_silent=0;

		for(int i=0;i<pl->count();i++)
		{
			//test cases |0  |1  01  10  00  11

			//input
			pli =  pl->get_at_index(i);

			//output
			if(pli_merged==NULL) // |0, |1
			{
				//init
				pli_merged = new PlaylistItem();

				pli_merged->nid=pli->nid;

				pli_merged->ev=new Event();

				pli_merged->ev->start_linear=pli->ev->start_linear;
				pli_merged->ev->start=pli->ev->start;
				pli_merged->ev->silent=pli->ev->silent;
				pli_merged->ev->count=pli->ev->count;

				pli_merged->ev->file_id=pli->ev->file_id;
			}

			if(!prev_was_silent && !pli->ev->silent) //00
			{
				prev_was_silent=0;
				unified_silent_count=0;
				//add new
			}
			else if(!prev_was_silent && pli->ev->silent) //01
			{
				prev_was_silent=1;
				unified_silent_count=pli->ev->count;
				//add new
			}
			else if(prev_was_silent && !pli->ev->silent) //10
			{
				//set final unified count
				pli_merged->ev->count=unified_silent_count;

				prev_was_silent=0;
				unified_silent_count=0;
				//add new
			}
			else if(prev_was_silent && pli->ev->silent) //11
			{
				unified_silent_count+=pli->ev->count;
				//if last
				if(i==pl->count()-1)
				{
					//set final unified count
					pli_merged->ev->count=unified_silent_count;
				}
				continue;
			}

			pli_merged=new PlaylistItem();

			pli_merged->nid=pli->nid;

			pli_merged->ev=new Event();

			pli_merged->ev->start_linear=pli->ev->start_linear;
			pli_merged->ev->start=pli->ev->start;
			pli_merged->ev->silent=pli->ev->silent;
			pli_merged->ev->count=pli->ev->count;
			pli_merged->ev->file_id=pli->ev->file_id;

			add(pli_merged);

		}//end for
	}//end update()

	//=======================================================================
	MergedPlaylist(Playlist *pl)
	{
		update(pl);
	}

	//=======================================================================
	void print()
	{
		if(plitems_counter==0)
		{
			fprintf(stderr,"//===merged playlist: empty\n");
			return;
		}
		//header
		fprintf(stderr,"//===merged playlist: %"PRId64" items (%"PRId64")\n//[index] pos silent count (fid start; nid)\n"
			,plitems_counter
//			,plitems[0]->ev->start_linear
			,end()
		);
		for(uint64_t i=0;i<plitems_counter;i++)
		{
			plitems[i]->print(i);
		}
		fprintf(stderr,"\n");
	}

};//end struct MergedPlaylist

#endif 
//EOF
