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

#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

#include "common.h"

//incremented at struct creation
uint64_t Event_serial_id_counter=0;

//defined in timeline.h
int add_to_playlist(uint64_t start_linear, uint64_t nid, uint64_t start, uint64_t count, int silent,uint64_t file_id);

//===============================================================================
//===============================================================================
struct Event
{
	uint64_t id_serial; //auto inc
	uint64_t start;
	uint64_t count;

	int silent;
	int cut; //"delete time"

	int additional; //"add time", in comb. with silent

	uint64_t file_id;

	uint64_t start_linear; //set on eval

	//=======================================================================
	Event()
	{
		id_serial=Event_serial_id_counter;
		Event_serial_id_counter++;

		start=0;
		count=0;

		silent=0;
		cut=0;

		additional=0;

		file_id=0;

		start_linear=0;

//		fprintf(stderr,"created Event id %"PRId64"\n",id_serial);
	}

	//=======================================================================
	uint64_t end()
	{
		return start+count;
	}

	//=======================================================================
	int is_empty()
	{
		return count==0;
	}

	//=======================================================================
	void eval(int do_print, int level, int is_leaf, uint64_t node_id
		,uint64_t &_linear_counter, uint64_t &_remove_counter, uint64_t &_add_counter
	)
	{
		start_linear=_linear_counter;

		if(do_print)
		{
			for(int i=0;i<level;i++)
			{
				fprintf(stderr,"  ");
			}
			fprintf(stderr,"  ");

			fprintf(stderr,"<event id=\"%"PRId64"\" nid=\"%"PRId64"\" leaf=\"%d\" ",id_serial,node_id,is_leaf);
			fprintf(stderr,"fid=\"%"PRId64"\" ",file_id);
			fprintf(stderr,"empty=\"%d\" ",is_empty());
			fprintf(stderr,"silent=\"%d\" ",silent);
			fprintf(stderr,"additional=\"%d\" ",additional);
			fprintf(stderr,"cut=\"%d\" ",cut);
			fprintf(stderr,"start_linear=\"%"PRId64"\" ",start_linear);
			fprintf(stderr,"start=\"%"PRId64"\" ",start);
			fprintf(stderr,"count=\"%"PRId64"\" ",count);
			fprintf(stderr,"remove_counter=\"%"PRId64"\" ",_remove_counter);
			fprintf(stderr,"add_counter=\"%"PRId64"\" ",_add_counter);
			fprintf(stderr,"/>\n");
		}

		if(is_leaf && !is_empty() && !cut)
		{
			add_to_playlist(start_linear,node_id,start,count,silent,file_id);
			_linear_counter+=count;

		}

		if(cut)
		{
			_remove_counter+=count;

		}

//		if(silent && additional)
		if(additional)
		{
			_add_counter+=count;
		}
	}//end eval()
};//end struct Event

#endif
//EOF
